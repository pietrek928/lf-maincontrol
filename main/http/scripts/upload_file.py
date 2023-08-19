# Upload to SD card one specific file
# Usage: python upload_file.py --esp_url http://10.10.128.1/write_file/ --file data.txt

from asyncio import run
from aiohttp import ClientSession

from click import command, option, File

async def upload_file(esp_url, filename, data):                        
    async with ClientSession() as session:
        async with session.post(f'{esp_url}{filename}', data=b'wb' + data) as resp:
            resp.raise_for_status()
            print('Uploaded result', await resp.text())


# Usage: python upload_file.py --esp_url http://10.10.128.1/write_file/ --file data.txt
@command()
@option('--esp_url', type=str)
@option('--file', type=File('rb'))
def upload( esp_url, file):
    app_data = file.read()
    filename = file.name
    print(filename)
    run(upload_file(esp_url, filename, app_data))
    
       
if __name__ == '__main__':
    upload()







