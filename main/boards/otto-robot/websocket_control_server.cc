#include "websocket_control_server.h"
#include "mcp_server.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <sys/param.h>
#include <cstring>
#include <cstdlib>
#include <map>

static const char* TAG = "WSControl";

WebSocketControlServer* WebSocketControlServer::instance_ = nullptr;

WebSocketControlServer::WebSocketControlServer() : server_handle_(nullptr) {
    instance_ = this;
}

WebSocketControlServer::~WebSocketControlServer() {
    Stop();
    instance_ = nullptr;
}

esp_err_t WebSocketControlServer::ws_handler(httpd_req_t *req) {
    if (instance_ == nullptr) {
        return ESP_FAIL;
    }
    
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Bắt tay hoàn tất, kết nối mới đã được mở");
        instance_->AddClient(req);
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    /* Đặt max_len = 0 để lấy độ dài frame */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame thất bại khi lấy độ dài frame: %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Độ dài frame: %d", ws_pkt.len);
    
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 để kết thúc chuỗi NULL vì đang nhận dữ liệu dạng text */
        buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Cấp phát bộ nhớ cho buf thất bại");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Đặt max_len = ws_pkt.len để nhận payload của frame */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame thất bại: %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Nhận gói tin với nội dung: %s", ws_pkt.payload);
    }
    
    ESP_LOGI(TAG, "Loại gói tin: %d", ws_pkt.type);
    
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "Đã nhận frame đóng WebSocket");
        instance_->RemoveClient(req);
        free(buf);
        return ESP_OK;
    }
    
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (ws_pkt.len > 0 && buf != nullptr) {
            buf[ws_pkt.len] = '\0';
            instance_->HandleMessage(req, (const char*)buf, ws_pkt.len);
        }
    } else {
        ESP_LOGW(TAG, "Loại frame không hỗ trợ: %d", ws_pkt.type);
    }
    
    free(buf);
    return ESP_OK;
}

bool WebSocketControlServer::Start(int port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_open_sockets = 7;
    config.ctrl_port = 32769;

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = nullptr,
        .is_websocket = true
    };

    if (httpd_start(&server_handle_, &config) == ESP_OK) {
        httpd_register_uri_handler(server_handle_, &ws_uri);
        ESP_LOGI(TAG, "WebSocket server đã khởi động trên cổng %d", port);
        return true;
    }

    ESP_LOGE(TAG, "Khởi động WebSocket server thất bại");
    return false;
}

void WebSocketControlServer::Stop() {
    if (server_handle_) {
        httpd_stop(server_handle_);
        server_handle_ = nullptr;
        clients_.clear();
        ESP_LOGI(TAG, "WebSocket server đã dừng");
    }
}

void WebSocketControlServer::HandleMessage(httpd_req_t *req, const char* data, size_t len) {
    if (data == nullptr || len == 0) {
        ESP_LOGE(TAG, "Tin nhắn không hợp lệ: data null hoặc len bằng 0");
        return;
    }
    
    if (len > 4096) {
        ESP_LOGE(TAG, "Tin nhắn quá dài: %zu byte", len);
        return;
    }
    
    char* temp_buf = (char*)malloc(len + 1);
    if (temp_buf == nullptr) {
        ESP_LOGE(TAG, "Cấp phát bộ nhớ thất bại");
        return;
    }
    memcpy(temp_buf, data, len);
    temp_buf[len] = '\0';
    
    cJSON* root = cJSON_Parse(temp_buf);
    free(temp_buf);
    
    if (root == nullptr) {
        ESP_LOGE(TAG, "Phân tích JSON thất bại");
        return;
    }

    // Hỗ trợ hai định dạng:
    // 1. Định dạng đầy đủ: {"type":"mcp","payload":{...}}
    // 2. Định dạng rút gọn: trực tiếp là đối tượng MCP payload
    
    cJSON* payload = nullptr;
    cJSON* type = cJSON_GetObjectItem(root, "type");
    
    if (type && cJSON_IsString(type) && strcmp(type->valuestring, "mcp") == 0) {
        payload = cJSON_GetObjectItem(root, "payload");
        if (payload != nullptr) {
            cJSON_DetachItemViaPointer(root, payload);
            McpServer::GetInstance().ParseMessage(payload);
            cJSON_Delete(payload); 
        }
    } else {
        payload = cJSON_Duplicate(root, 1);
        if (payload != nullptr) {
            McpServer::GetInstance().ParseMessage(payload);
            cJSON_Delete(payload);
        }
    }
    
    if (payload == nullptr) {
        ESP_LOGE(TAG, "Định dạng tin nhắn không hợp lệ hoặc phân tích thất bại");
    }

    cJSON_Delete(root);
}

void WebSocketControlServer::AddClient(httpd_req_t *req) {
    int sock_fd = httpd_req_to_sockfd(req);
    if (clients_.find(sock_fd) == clients_.end()) {
        clients_[sock_fd] = req;
        ESP_LOGI(TAG, "Client kết nối: %d (tổng: %zu)", sock_fd, clients_.size());
    }
}

void WebSocketControlServer::RemoveClient(httpd_req_t *req) {
    int sock_fd = httpd_req_to_sockfd(req);
    clients_.erase(sock_fd);
    ESP_LOGI(TAG, "Client ngắt kết nối: %d (tổng: %zu)", sock_fd, clients_.size());
}

size_t WebSocketControlServer::GetClientCount() const {
    return clients_.size();
}