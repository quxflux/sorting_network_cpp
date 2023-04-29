import os

from os import listdir

def _read_and_filter_network_impl_file(path:str):
    lines = open(path, 'r', encoding='utf8').read().splitlines()
    first_line = next(x for x in enumerate(lines) if x[1].startswith('namespace quxflux::sorting_net::detail'))[0]
    return '\n'.join(lines[first_line:] + ['\n'])

dir_path = os.path.dirname(os.path.realpath(__file__))

out_path = os.path.join(dir_path, '..', 'single_header_impl')
include_dir_path = os.path.join(dir_path, '..', 'include', 'sorting_network_cpp', 'networks')

joined_contents = open(os.path.join(include_dir_path, 'common.h'), 'r', encoding='utf8').read() + '\n'

all_files = [f for f in listdir(include_dir_path) if not f in set(['common.h', 'all.h'])]

for f in all_files:
    joined_contents += _read_and_filter_network_impl_file(os.path.join(include_dir_path, f))

open(os.path.join(out_path, 'sorting_network_cpp.h'), 'w', encoding='utf8').write(joined_contents)
