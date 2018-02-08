#!/usr/bin/env python

import os
import sys

from lib.config import output_dir, get_electron_version


def main():
  create_version()


def create_version():
  version_path = os.path.join(output_dir(), 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(get_electron_version())


if __name__ == '__main__':
  sys.exit(main())
