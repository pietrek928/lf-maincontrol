# Flash the ESP32 through WiFi based on .bin file from project
# Usage: python main/http/flasher/reflash_app.py --esp_url http://10.10.128.1 --app_file build/esp_wifi.bin
from asyncio import run
from aiohttp import ClientOSError, ClientSession
from struct import pack

from click import command, option, File

def name_to_bin(name):
    name = name.encode('utf-8')
    return name + b'\0' * (20-len(name))

ESP_APP_PARTITION_TYPE = 0

async def partition_info(esp_url):
    async with ClientSession() as session:
        async with session.get(f'{esp_url}/flasher/partition_info') as resp:
            resp.raise_for_status()
            return await resp.json()

async def write_data(esp_url, partition_name, data):
    flash_data = name_to_bin(partition_name) + pack('I', len(data))
    async with ClientSession() as session:
        async with session.post(f'{esp_url}/flasher/write_ota', data=flash_data + data) as resp:
            resp.raise_for_status()
            print(f'Flash write of {len(data)} bytes complete at partition name {partition_name}')

def find_next_app_partition(partition_data, needed_size):
    running_partition = partition_data['ota'].get('running_partition')
    if not running_partition:
        raise ValueError('No running partition info - is there ota?')

    for p in partition_data['partitions']:
        if p['type'] == ESP_APP_PARTITION_TYPE and p['label'] != running_partition and p['size'] >= needed_size:
            return p

    raise ValueError(f'No suitable app partition found for {needed_size} bytes')

async def set_boot_partition(esp_url, partition_label):
    print(f'Setting boot partition to {partition_label}')
    async with ClientSession() as session:
        async with session.post(
            f'{esp_url}/flasher/set_boot_partition',
            data=name_to_bin(partition_label)
        ) as resp:
            resp.raise_for_status()

async def reboot_esp(esp_url):
    print('Rebooting ESP')
    try:
        async with ClientSession() as session:
            async with session.post(f'{esp_url}/flasher/reboot') as resp:
                resp.raise_for_status()
    except ClientOSError:
        print("ESP rebooted sucesfully")

async def reflash_app_async(esp_url, app_data):
    partition_data = await partition_info(esp_url)
    app_partition = find_next_app_partition(partition_data, len(app_data))
    print(f'Writing app to {app_partition["label"]} partition at offset {app_partition["address"]}')
    await write_data(esp_url, app_partition['label'], app_data)
    await set_boot_partition(esp_url, app_partition['label'])
    await reboot_esp(esp_url)

@command()
@option('--esp_url', type=str)
@option('--app_file', type=File('rb'))
def reflash_app(esp_url, app_file):
    app_data = app_file.read()
    run(reflash_app_async(esp_url, app_data))

if __name__ == '__main__':
    reflash_app()
