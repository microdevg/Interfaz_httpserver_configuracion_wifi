
#include <stdbool.h>
typedef void(*callback_t)(void);


void wifi_set_cb_app(callback_t cb);

void wifi_set_cb_error(callback_t cb);

// Luego debe liberar memoria asignada
bool* wm_start( char** id,  char** pass);