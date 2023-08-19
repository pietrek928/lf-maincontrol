#include "ethernet.h"

const char* TAG_eth = "ethernet controller";

/* Global handlers */
esp_netif_t* eth_netif = NULL;
esp_eth_phy_t* eth_phy = NULL;
esp_eth_mac_t* eth_mac = NULL;
esp_eth_handle_t eth_handle = NULL;

constexpr static eth_esp32_emac_config_t esp32_emac_config = {
    .smi_mdc_gpio_num = 23,
    .smi_mdio_gpio_num = 18,
    .interface = EMAC_DATA_INTERFACE_RMII,
    .clock_config =
        {.rmii =
             {
                 .clock_mode = EMAC_CLK_OUT,
                 .clock_gpio = EMAC_CLK_OUT_180_GPIO,
             }},
    .dma_burst_len = ETH_DMA_BURST_LEN_32,
};
constexpr static eth_mac_config_t mac_config = {
    .sw_reset_timeout_ms = 100,
    .rx_task_stack_size = 2048,
    .rx_task_prio = 15,
    .flags = 0,
};
constexpr static eth_phy_config_t phy_config = {
    .phy_addr = ESP_ETH_PHY_ADDR_AUTO,
    .reset_timeout_ms = 100,
    .autonego_timeout_ms = 4000,
    .reset_gpio_num = 5,
};

void ethernet_init()
{
    if (!eth_netif)
    {
        esp_netif_config_t eth_netif_cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netif = esp_netif_new(&eth_netif_cfg);
        if (!eth_netif)
        {
            ESP_LOGE(TAG_eth, "Failed to create ethernet network interface");
            return;
        }
    }

    if (!eth_mac)
    {
        eth_mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
        if (!eth_mac)
        {
            ESP_LOGE(TAG_eth, "Failed to create MAC");
            return;
        }
    }
    if (!eth_phy)
    {
        eth_phy = esp_eth_phy_new_lan87xx(&phy_config);
        if (!eth_phy)
        {
            ESP_LOGE(TAG_eth, "Failed to create PHY");
            return;
        }
    }
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(eth_mac, eth_phy);
    esp_err_t driver_install_result =
        esp_eth_driver_install(&config, &eth_handle);
    if (driver_install_result != ESP_OK)
    {
        ESP_LOGE(
            TAG_eth,
            "Failed to install ethernet driver: %s",
            esp_err_to_name(driver_install_result));
        return;
    }

    esp_err_t attach_result =
        esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));

    if (attach_result != ESP_OK)
    {
        ESP_LOGE(
            TAG_eth,
            "Failed to attach ethernet driver: %s",
            esp_err_to_name(attach_result));
        return;
    }
}

void ethernet_start() {
    esp_err_t start_result = esp_eth_start(eth_handle);
    if (start_result != ESP_OK)
    {
        ESP_LOGE(
            TAG_eth,
            "Failed to start ethernet driver: %s",
            esp_err_to_name(start_result));
        return;
    }
}

void ethernet_stop() {
    esp_err_t stop_result = esp_eth_stop(eth_handle);
    if (stop_result != ESP_OK)
    {
        ESP_LOGE(
            TAG_eth,
            "Failed to stop ethernet driver: %s",
            esp_err_to_name(stop_result));
        return;
    }
}

esp_netif_ip_info_t ethernet_ip_info() {
    esp_netif_ip_info_t ip_info;
    if (eth_netif) {
        esp_netif_get_ip_info(eth_netif, &ip_info);
    }
    return ip_info;
}
