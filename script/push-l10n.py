import argparse
import os
import sys
import tempfile
import xml.etree.ElementTree
from lib.config import get_env_var
from lib.transifex import upload_source_string_file_to_transifex, generate_source_strings_xml_from_grd, get_transifex_languages, get_grd_strings, check_for_chromium_upgrade, check_missing_source_grd_strings_to_transifex


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)


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


def upload_source_files_to_transifex(grd_file_path, filename):
  # Generate the intermediate Transifex format for the source translations
  output_xml_file_handle, output_xml_path = tempfile.mkstemp('.xml')
  xml_content = generate_source_strings_xml_from_grd(output_xml_file_handle, grd_file_path)

  # Update Transifex
  uploaded = upload_source_string_file_to_transifex(filename, xml_content)
  os.close(output_xml_file_handle)
  if not uploaded:
    sys.exit('Could not upload xml file')


def main():
  args = parse_args()
  check_args()

  grd_file_path = os.path.join(BRAVE_SOURCE_ROOT, args.grd_path[0])
  filename = os.path.basename(grd_file_path).split('.')[0]

  upload_source_files_to_transifex(grd_file_path, filename)
  check_for_chromium_upgrade(SOURCE_ROOT, grd_file_path)
  check_missing_source_grd_strings_to_transifex(grd_file_path)


if __name__ == '__main__':
  sys.exit(main())
