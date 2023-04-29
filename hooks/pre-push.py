import os
import subprocess

script_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'scripts', 'make_single_header_impl.py')
subprocess.Popen(f'py {script_path}', stdout=subprocess.PIPE, shell=True)
