import json
from sys import argv
from numpy import fromfile
from pandas import DataFrame

def dtype_from_meta(log_meta):
    types = []
    for name, type_descr in zip(log_meta['names'].split(','), log_meta['types']):
        match type_descr['type']:
            case 'uint8_t': numpy_type = '<u1'
            case 'int8_t': numpy_type = '<i1'
            case 'uint16_t': numpy_type = '<u2'
            case 'int16_t': numpy_type = '<i2'
            case 'uint32_t': numpy_type = '<u4'
            case 'int32_t': numpy_type = '<i4'
            case 'float': numpy_type = 'f4'
            case 'double': numpy_type = 'f8'
            case _: raise NotImplementedError(f'Unknown type {type_descr["type"]}')
        types.append((name.strip(), numpy_type))
    return types


def load_log_file(fname):
    with open(f'{fname}.json', 'r') as f:
        descr_data = json.load(f)
    data = fromfile(
        f'{fname}.blog', dtype=dtype_from_meta(descr_data)
    )
    return DataFrame(data, columns=data.dtype.names)

if __name__ == '__main__':
    fname = argv[1].rsplit('.')[0]
    load_log_file(fname).to_csv(f'{fname}.csv', index=False)
