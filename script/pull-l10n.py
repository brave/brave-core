import argparse
import os.path
from os import walk
import sys
import xml.etree.ElementTree
from lib.config import get_env_var
from lib.transifex import get_xtb_files, get_grd_strings, textify, get_transifex_translation_file_content, get_strings_dict_from_xml_content, generate_xtb_content


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
  grd_file_path = os.path.join(SOURCE_ROOT, args.grd_path[0])
  filename = os.path.basename(grd_file_path).split('.')[0]

  grd_strings = get_grd_strings(grd_file_path)

  print '-----------'
  print 'Source GRD:', grd_file_path
  print 'Transifex resource slug: ', filename

  # Generate the intermediate Transifex format
  xtb_files = get_xtb_files(grd_file_path)
  base_path = os.path.dirname(grd_file_path)
  for (lang_code, xtb_rel_path) in xtb_files:
    xtb_file_path = os.path.join(base_path, xtb_rel_path)
    xml_content = get_transifex_translation_file_content(filename, lang_code)
    translations = get_strings_dict_from_xml_content(xml_content)
    xtb_content = generate_xtb_content(grd_strings, translations)
    print 'Updated: ', xtb_file_path
    f = open(xtb_file_path, 'w')
    f.write(xtb_content)
    f.close()


if __name__ == '__main__':
  sys.exit(main())
