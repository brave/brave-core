"""Generates Obj-C++ source files from a mojom.Module."""

import argparse

import importlib
import os
import sys

_current_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(1, os.path.join(_current_dir, *([os.pardir] * 4 + ['mojo/public/tools/mojom'])))

import mojom.fileutil as fileutil
from mojom.generate import template_expander
from mojom.generate.module import Module

def parse_args():
  parser = argparse.ArgumentParser(description='Generate Obj-C files from mojo definitions')
  parser.add_argument('--mojom-module', nargs=1)
  parser.add_argument('--output-dir', nargs=1)
  parser.add_argument('--class-prefix', nargs='?', default="")
  parser.add_argument('--exclude', nargs='*', default=[])
  return parser.parse_args()

def main():
  global ast_root_dir

  args = parse_args()
  mojom_module = args.mojom_module[0]
  output_dir = args.output_dir[0]
  class_prefix = args.class_prefix
  excluded = args.exclude

  ast_root_dir = os.path.dirname(mojom_module)

  generator_module = importlib.import_module('mojom_objc_generator')
  bytecode_path = os.path.join(output_dir, "objc_templates_bytecode")
  fileutil.EnsureDirectoryExists(bytecode_path)
  template_expander.PrecompileTemplates({"objc": generator_module},
                                        bytecode_path)
  generator = generator_module.Generator(None)
  if len(class_prefix) > 0:
    generator.class_prefix = class_prefix
  generator.bytecode_path = bytecode_path
  generator.excludedTypes = excluded
  with open(mojom_module, 'rb') as f:
    generator.module = Module.Load(f)
  generator.GenerateFiles(output_dir)

if __name__ == "__main__":
  sys.exit(main())
