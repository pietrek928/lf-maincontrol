# Build front web project int to web.h file for downloading web from flash
# python files2code.py ../../front/build > web.h
from sys import argv, stdout, stderr
import re
import gzip
from os import path
from glob import glob
from mimetypes import guess_type


def render_includes(*files, local=False):
    """
    Generator includów
    """
    for file in files:
        if local:
            print(f'#include "{file}"')
        else:
            print(f'#include <{file}>')


def render_array(dtype, name, items):
    """
    Generuje w C++ tablice
    """
    items = tuple(items)
    print(f'static const {dtype} {name}[] = {{')
    for i in range(0, len(items), 16):
        print(' '.join(f'{item},' for item in items[i:i+16]))
    print(f'}};')


def render_bytes_array(name, data):
    render_array('uint8_t', name, (
        hex(c).rjust(4, " ") for c in data
    ))


def to_var_name(n):
    return re.sub(r'[./\\\-+=!?]', '_', n)


def render_files_lookup(fnames):
    render_includes('cstdint')

    lens = []
    arrays = []
    file_mimes = []
    loaded_files = {}
    mimetypes = set()
    for it, (path_name, fname) in enumerate(fnames):
        if path_name:
            print(f"Packaging {fname}...", file=stderr)
            if path_name in loaded_files:
                file_num = loaded_files[path_name]
                lens.append(lens[file_num])
                arrays.append(arrays[file_num])
                file_mimes.append(file_mimes[file_num])
            else:
                loaded_files[path_name] = it
                var_name = to_var_name(path_name)
                with open(fname, 'rb') as f:
                    data = gzip.compress(f.read())

                file_mime_type = guess_type(fname)[0]
                if file_mime_type is None:
                    file_mime_type = 'application/octet-stream'
                mime_var = to_var_name(file_mime_type)
                if mime_var not in mimetypes:
                    mimetypes.add(mime_var)
                    render_bytes_array(
                        mime_var + '_mime', file_mime_type.encode('utf-8') + b'\0'
                    )

                lens.append(len(data))
                arrays.append(var_name)
                file_mimes.append(mime_var)
                render_bytes_array(var_name + '_data', data)
                render_bytes_array(
                    var_name + '_name', path_name.encode('utf-8') + b'\0'
                )
    render_array('uint32_t', 'file_lengths', lens)
    render_array('uint8_t *', 'file_pointers', (
        a+'_data' for a in arrays
    ))
    render_array('uint8_t *', 'file_names', (
        mime_var+'_name' for mime_var in arrays
    ))
    render_array('uint8_t *', 'file_mimes', (
        mime_var+'_mime' for mime_var in file_mimes
    ))

    print(f'''
struct find_file_return_t{{
        const uint8_t *data = NULL;
        const uint8_t *mime_type = NULL;
        uint32_t len = -1;
    }};
find_file_return_t find_file(const uint8_t *name_lookup) {{
    find_file_return_t file_data;
    for (int i=0; i<{len(arrays)}; i++) {{
        auto name_ptr = file_names[i];
        auto name_lookup_ = name_lookup;
        do {{
            if (*name_lookup_ != *name_ptr) {{
                break;
            }}
            name_lookup_ ++;
            name_ptr ++;
        }} while (*name_lookup_ && *name_ptr);
        if (!*name_ptr && !*name_lookup_) {{
            file_data.data = file_pointers[i];
            file_data.len = file_lengths[i];
            file_data.mime_type = file_mimes[i];
            return file_data;
        }}
    }}
    return file_data;
}}
    ''')


base_path = argv[1]

render_files_lookup([["/", f"{base_path}/index.html"]] + sorted(
    (fname[len(base_path):].replace('\\', '/'), fname.replace('\\', '/'))
    for fname in glob(f'{base_path}/**/*', recursive=True)
    if path.isfile(fname)
))

#  python files2code.py ../../front/build > web.h
