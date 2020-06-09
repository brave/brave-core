import argparse
import os
import sys
import xml.etree.ElementTree
from lib.config import get_env_var
from lib.transifex import (check_for_chromium_upgrade,
                           upload_source_strings_desc,
                           check_missing_source_grd_strings_to_transifex,
                           upload_missing_json_translations_to_transifex,
                           upload_source_files_to_transifex,
                           should_use_transifex)


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path',
                        nargs=1)
    return parser.parse_args()


def check_args():
    transifex_info = (get_env_var('TRANSIFEX_USERNAME') and
                      get_env_var('TRANSIFEX_PASSWORD') or
                      get_env_var('TRANSIFEX_API_KEY'))
    message = 'BRAVE_TRANSIFEX_USERNAME and BRAVETRANSIFEX_PASSWORD or '
    'BRAVE_TRANSIFEX_API_KEY must be set in npm config'
    assert transifex_info, message


def main():
    args = parse_args()
    check_args()

    source_string_path = os.path.join(BRAVE_SOURCE_ROOT,
                                      args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]
    if not should_use_transifex(source_string_path, filename):
        print'Handled locally, not sending to Transifex: ', source_string_path
        return

    print '[transifex]: ', source_string_path
    upload_source_files_to_transifex(source_string_path, filename)
    ext = os.path.splitext(source_string_path)[1]
    if ext == '.grd':
        check_for_chromium_upgrade(SOURCE_ROOT, source_string_path)
        check_missing_source_grd_strings_to_transifex(source_string_path)
    upload_source_strings_desc(source_string_path, filename)
    if 'ethereum-remote-client' in source_string_path:
        upload_missing_json_translations_to_transifex(source_string_path)


if __name__ == '__main__':
    sys.exit(main())
