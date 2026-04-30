#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/ip4_addr.h"
#include "lwip/netif.h"

#include "dhcpserver.h"
#include "dnsserver.h"
#include "wifi_ap.h"

#define AP_SSID     "projeto_matematica"
#define AP_PASSWORD "123456789"

static dhcp_server_t dhcp_server;
static dns_server_t dns_server;

void wifi_ap_init(void) {
    cyw43_arch_enable_ap_mode(
        AP_SSID,
        AP_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK
    );

    ip4_addr_t ip;
    ip4_addr_t mask;

    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    netif_set_addr(netif_default, &ip, &mask, &ip);

    dhcp_server_init(&dhcp_server, &ip, &mask);
    dns_server_init(&dns_server, &ip);

    printf("\n=== WIFI AP ATIVO ===\n");
    printf("Rede: %s\n", AP_SSID);
    printf("Senha: %s\n", AP_PASSWORD);
    printf("Acesse: http://192.168.4.1\n");
    printf("=====================\n\n");
}