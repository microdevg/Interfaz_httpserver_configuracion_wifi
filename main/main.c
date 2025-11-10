#include <stdio.h>
#include "wifi_manager.h"
#include <unistd.h>


void app(void){
    printf("Everthing ok\n");
}

void error (void){
    printf("Error\n");



}
void app_main(void)
{
    char *ssid = NULL;
    char *password = NULL;
    bool *ok = wm_start(&ssid, &password);

   
    while(!(*ok)){
        sleep(1);
        printf("x");
    }

    printf("SSID: %s\n", ssid);
    printf("PASS: %s\n", password);
    free(ssid);
    free(password);
}