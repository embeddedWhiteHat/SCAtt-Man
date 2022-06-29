#include "http_server.h"
#include "nvslib.h"

static const char *HTTP_SERVER_TAG = "http server";

/* An HTTP POST handler */
esp_err_t http_conf_post_handler(httpd_req_t *req);

static const httpd_uri_t http_conf = {
    .uri       = "/http_conf",
    .method    = HTTP_POST,
    .handler   = http_conf_post_handler,
    .user_ctx  = NULL
};

void disconnect_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data);

void connect_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data);


#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);

/* An HTTP POST handler */
esp_err_t http_conf_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(HTTP_SERVER_TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(HTTP_SERVER_TAG, "%.*s", ret, buf);
        ESP_LOGI(HTTP_SERVER_TAG, "====================================");
    }

    if (nvslib_begin())
    {
        uint8_t *sha = (uint8_t *) calloc(32, sizeof(uint8_t));
        ESP_LOGI(HTTP_SERVER_TAG, "=========== NVS HASH BEFORE SAVING ==========");
        nvslib_get_partition_sha256(sha, true);
        nvslib_save_blob("http_conf", (uint8_t *) buf, sizeof(buf), false);
        ESP_LOGI(HTTP_SERVER_TAG, "=========== NVS HASH AFTER SAVING ==========");
        nvslib_get_partition_sha256(sha, true);
        nvslib_close();
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    else if (strcmp("/echo", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(HTTP_SERVER_TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(HTTP_SERVER_TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &http_conf);
        return server;
    }

    ESP_LOGI(HTTP_SERVER_TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(HTTP_SERVER_TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void connect_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(HTTP_SERVER_TAG, "Starting webserver");
        *server = start_webserver();
    }
}
