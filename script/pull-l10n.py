import argparse
import os.path
from os import walk
import sys
import xml.etree.ElementTree
from lib.config import get_env_var
from lib.transifex import pull_source_files_from_transifex, should_use_transifex, pull_xtb_without_transifex

BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path', nargs=1)
    return parser.parse_args()


def check_args():
    transifex_info = (get_env_var('TRANSIFEX_USERNAME') and
                      get_env_var('TRANSIFEX_PASSWORD') or
                      get_env_var('TRANSIFEX_API_KEY'))
    message = 'TRANSIFEX_USERNAME and TRANSIFEX_PASSWORD or '
    'TRANSIFEX_API_KEY must be set'
    assert transifex_info, message


def main():
    args = parse_args()
    check_args()
    source_string_path = os.path.join(BRAVE_SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]
    if should_use_transifex(source_string_path, filename):
        print('Transifex: ', source_string_path)
        pull_source_files_from_transifex(source_string_path, filename)
    else:
        print('Local: ', source_string_path)
        pull_xtb_without_transifex(source_string_path, BRAVE_SOURCE_ROOT)


if __name__ == '__main__':
    sys.exit(main())
