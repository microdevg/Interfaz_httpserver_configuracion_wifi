#include <stdio.h>
#include "wifi_manager.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <stdbool.h>




#define NAME_ESP_WIFI       "WMESP32"
#define PASS_ESP_WIFI       ""      // Si no hay contraseña se configura como open.



const char* TAG = "Wifi Manager";
extern const char home_page[]   asm("_binary_home_html_start");
extern const char confirmation_page[]   asm("_binary_confirmation_html_start");
extern const char indexjs[]  asm("_binary_index_js_start");
extern const char styles[]  asm("_binary_styles_css_start");



static httpd_handle_t server = NULL;



// Esta en false hasta que cargamos las credenciales
static char** _id = NULL;
static char** _pass = NULL;
static bool credential_flags = false;

/**
 * @brief Handler para manejar las solicitudes POST a la URI /configurar.
 * Extrae el SSID y el Password del formulario y actualiza las credenciales.
 */
static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char content[200];
    int ret;
    int remaining = req->content_len;

    // 1. Leer los datos del cuerpo del POST
    if (remaining >= sizeof(content)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, content, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0'; // Asegurar terminación de cadena

    ESP_LOGI(TAG, "Datos POST recibidos: %s", content);

    // 2. Parsear los datos esperados: "wifi-id=MiRed&wifi-password=MiPass"
    char *ssid_start = strstr(content, "wifi-id=");
    char *pass_start = strstr(content, "wifi-password=");

    if (ssid_start && pass_start) {
        char *ssid = ssid_start + strlen("wifi-id=");
        char *ssid_end = strchr(ssid, '&');
        if (ssid_end) *ssid_end = '\0';

        char *password = pass_start + strlen("wifi-password=");

        ESP_LOGI(TAG, "SSID extraído: %s", ssid);
        ESP_LOGI(TAG, "Password extraído: %s", password);

        // 3. Guardar los datos en los punteros globales si existen
        if (_id && _pass) {
            // Liberar posibles buffers anteriores (opcional, si se reutiliza)
            if (*_id) free(*_id);
            if (*_pass) free(*_pass);

            // Reservar memoria y copiar el contenido
            *_id = strdup(ssid);
            *_pass = strdup(password);

            if (*_id && *_pass) {
                credential_flags = true;
                ESP_LOGI(TAG, "Credenciales almacenadas correctamente.");
            } else {
                ESP_LOGE(TAG, "Error al asignar memoria para credenciales.");
                credential_flags = false;
            }
        } else {
            ESP_LOGE(TAG, "_id o _pass no inicializados en wm_start().");
        }

    } else {
        ESP_LOGE(TAG, "Error al parsear datos POST");
    }

    // 4. Redirigir al navegador
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/confirmation.html");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t home_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, home_page, strlen(home_page));
    return ESP_OK;
}


static esp_err_t confirmation_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, confirmation_page, strlen(confirmation_page));
    return ESP_OK;
}


static const httpd_uri_t home = {.uri = "/",.method = HTTP_GET,.handler = home_get_handler};
static const httpd_uri_t confirmation = {.uri = "/confirmation.html",.method = HTTP_GET,.handler = confirmation_get_handler};
// Estructura de la URI para registrar el handler
static const httpd_uri_t configuration_post = {
    .uri       = "/configurar",
    .method    = HTTP_POST,
    .handler   = wifi_config_post_handler,
    .user_ctx  = NULL
};





static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; // más holgado

    ESP_LOGI(TAG, "Iniciando servidor HTTP...");
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Servidor iniciado correctamente");
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &home));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &confirmation));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &configuration_post));
    } else {
        ESP_LOGE(TAG, "Error al iniciar el servidor HTTP");
    }

    return server;
}


// Manejador de eventos de red
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Punto de acceso iniciado");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ESP_LOGI(TAG, "Un cliente obtuvo IP, iniciando servidor HTTP...");
        if (server == NULL) {
            server = start_webserver();
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
        ESP_LOGI(TAG, "Punto de acceso detenido, deteniendo servidor HTTP...");
        if (server) {
            httpd_stop(server);
            server = NULL;
        }
    }
}









// Inicializar WiFi en modo Access Point
static void wifi_init_softap(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    // TCP/IP y event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registrar eventos WiFi e IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_AP_STAIPASSIGNED,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = NAME_ESP_WIFI,
            .ssid_len = strlen(NAME_ESP_WIFI),
            .channel = 1,
            .password = PASS_ESP_WIFI,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen((char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP iniciado. SSID:%s Password:%s Canal:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);
}







bool* wm_start( char** id,  char** pass){

    _id = id;
    _pass = pass;
    wifi_init_softap();
    start_webserver();

    return &credential_flags;
}


