# Upload web files to SD card to open the web
# Usage: python ./main/http/upload_files.py --esp_url http://10.10.128.1/write_file --input_dir front/build

from os.path import isdir
from asyncio import run
from os import listdir
from gzip import compress
from aiohttp import ClientSession
from click import command, option


GZIP_EXTENSIONS = {'html', 'css', 'js', 'json', 'txt', 'csv', 'xml', 'svg', 'md', 'map', 'ico', 'ttf', 'otf', 'eot', 'woff', 'woff2'}


# send single file to endpoint as post
async def upload_file(esp_url, file_name):
    print('Uploading file', file_name, ' -> ', esp_url)
    with open(file_name, 'rb') as f:
        file_content = f.read()

    file_ext = file_name.split('.')[-1]
    if file_ext in GZIP_EXTENSIONS:
        print('Compressing')
        file_content = compress(file_content)
        esp_url += '.gz'

    async with ClientSession() as session:
        async with session.post(esp_url, data=b'wb' + file_content) as resp:
            print('Sending data')
            resp.raise_for_status()
            print('Uploaded result', await resp.text())


# list files to upload
def list_files_to_upload(dir_path, esp_url):
    for name in listdir(dir_path):
        obj_path = f'{dir_path}/{name}'
        if isdir(obj_path):
            yield from list_files_to_upload(obj_path, f'{esp_url}/{name}')
        else:
            yield obj_path, f'{esp_url}/{name}'



@command()
@option('--esp_url', type=str)
@option('--input_dir', type=str)
def upload_files(esp_url, input_dir):
    for file_name, file_esp_url in list_files_to_upload(
        input_dir.replace('\\', '/').rstrip('/'), esp_url.rstrip('/')
    ):
        run(upload_file(file_esp_url, file_name))


if __name__ == '__main__':
    upload_files()
