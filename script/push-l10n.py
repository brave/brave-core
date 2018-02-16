import argparse
import os
import sys
import tempfile
import xml.etree.ElementTree
from lib.config import get_env_var
from lib.transifex import upload_source_string_file_to_transifex, generate_source_strings_xml_from_grd


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
  parser = argparse.ArgumentParser(description='Push strings to Transifex')
  parser.add_argument('--grd_path',
                      nargs=1)
  return parser.parse_args()


def check_args():
  transifex_info = (get_env_var('TRANSIFEX_USERNAME') and
          get_env_var('TRANSIFEX_PASSWORD') or get_env_var('TRANSIFEX_API_KEY'))
  message = 'TRANSIFEX_USERNAME and TRANSIFEX_PASSWORD or TRANSIFEX_API_KEY must be set'
  assert transifex_info, message


def main():
  args = parse_args()
  check_args()
  output_xml_file_handle, output_xml_path = tempfile.mkstemp('.xml')
  grd_path = os.path.join(SOURCE_ROOT, args.grd_path[0])
  filename = os.path.basename(grd_path).split('.')[0]

  # Generate the intermediate Transifex format
  xml_content = generate_source_strings_xml_from_grd(output_xml_file_handle, grd_path)

  # Update Transifex
  username = get_env_var('TRANSIFEX_USERNAME')
  password = get_env_var('TRANSIFEX_PASSWORD')
  transifex_api_key = get_env_var('TRANSIFEX_API_KEY')
  uploaded = upload_source_string_file_to_transifex(filename, xml_content, username, password, transifex_api_key)
  os.close(output_xml_file_handle)
  if not uploaded:
    sys.exit(1)


if __name__ == '__main__':
  sys.exit(main())
