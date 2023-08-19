#pragma once

#include <esp_eth.h>
#include <esp_eth_phy.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>


void ethernet_init();
void ethernet_start();
void ethernet_stop();
esp_netif_ip_info_t ethernet_ip_info();
