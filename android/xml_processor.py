#!/usr/bin/env python

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""XML processor"""

from pathlib import Path
import xml.etree.ElementTree as ET

import codecs
import argparse
import importlib.util
import os
import sys

sys.path.append(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, 'build'))
import zip_helpers  # pylint: disable=wrong-import-position

sys.path.append(os.path.join(os.path.dirname(__file__),
                             os.pardir, os.pardir,
                             'build', 'android', 'gyp'))
from util import build_utils  # pylint: disable=no-name-in-module,wrong-import-position
from util import resource_utils  # pylint: disable=no-name-in-module,wrong-import-position


def  _UnderJavaRes(source):
    """
  Check from left whether java/res is part of input path, returns relative
  path to java/res. Input path shall be absolute.
  """
    source_path = Path(source)
    source_path_parts = source_path.parts
    for i in range(1, len(source_path_parts)):
        if source_path_parts[i - 1] == 'java' and source_path_parts[i] == 'res':
            parent_path = source_path.parents[len(source_path_parts) - i - 2]
            try:
                rel_path = str(source_path.relative_to(parent_path))
                return rel_path
            except ValueError as e:
                print(e)
                return None
    return None


def _AddBravePrefix(relpath):
    dirname, filename = os.path.split(relpath)
    return os.path.join(dirname, 'brave_' + filename)


def _ImportModuleByPath(module_path):
    """Imports a module by its source file."""
    sys.path[0] = os.path.dirname(module_path)

    module_name = os.path.splitext(os.path.basename(module_path))[0]
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)

    return module


def _ProcessFile(output_filename, output):
    if os.path.isfile(output_filename):
        with codecs.open(output_filename, 'r', 'utf-8') as f:
            if f.read() == output:
                return

    with open(output_filename, 'wb') as output_file:
        output_file.write(output)


def _XMLTransform(source_pairs, outputs_zip):
    """
  Apply module codes to input sources.
  """
    with build_utils.TempDir() as temp_dir:
        path_info = resource_utils.ResourceInfoFile()
        for source, module in source_pairs:
            with codecs.open(source, 'r', 'utf-8') as f:
                xml_content = f.read()
            loaded_module = _ImportModuleByPath(module)

            if not xml_content or not loaded_module:
                continue

            root = ET.XML(xml_content)
            result = loaded_module._ProcessXML(root)  # pylint: disable=protected-access
            output = ET.tostring(result,
                                 encoding='utf-8',
                                 xml_declaration=True)

            # Parse output path
            # For simplicity, we assume input path will always has java/res in
            # it
            if not (relpath := _UnderJavaRes(os.path.abspath(source))):
                raise Exception('input file %s is not under java/res' % source)

            # resource_overlay doesn't seem to work from
            # android_generated_resources
            relpath = _AddBravePrefix(relpath)
            output_filename = os.path.join(temp_dir, relpath)
            parent_dir = os.path.dirname(output_filename)
            build_utils.MakeDirectory(parent_dir)
            _ProcessFile(output_filename, output)
            path_info.AddMapping(relpath, source)

        path_info.Write(outputs_zip + '.info')
        zip_helpers.zip_directory(outputs_zip, temp_dir)


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--xml-sources-path',
        required=True,
        help='Path to a list of input xml sources for this target.')
    parser.add_argument(
        '--module-sources-path',
        required=True,
        help='Path to a list of input python sources for this target. '
        'Each python file shall have a function named _ProcessXML.')
    parser.add_argument(
        '--outputs-zip',
        required=True,
        help='Path to a list of expected outputs for this target.')

    options = parser.parse_args(args)

    with open(options.xml_sources_path) as f:
        options.sources = f.read().splitlines()
    with open(options.module_sources_path) as f:
        options.modules = f.read().splitlines()

    assert len(options.sources) == len(options.modules)

    _XMLTransform(list(zip(options.sources, options.modules)),
                  options.outputs_zip)


if __name__ == '__main__':
    main(sys.argv[1:])
