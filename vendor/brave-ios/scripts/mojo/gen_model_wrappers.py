"""Generates Obj-C++ source files from a mojom.Module."""

import argparse

try:
  import cPickle as pickle
except ImportError:
  import pickle

import importlib
import os
import sys

_current_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(1, os.path.join(_current_dir, *([os.pardir] * 5 + ['mojo/public/tools/bindings/pylib'])))

import mojom.fileutil as fileutil
import mojom.generate.generator as generator
from mojom.generate import template_expander
from mojom.generate import translate

def _UnpickleAST(input_file):
  try:
    with open(input_file, "rb") as f:
      return pickle.load(f)
  except (IOError, pickle.UnpicklingError) as e:
    print("%s: Error: %s" % (input_file, str(e)))
    sys.exit(1)

def _GenerateModule(tree, module_path):
  imports = {}
  for parsed_imp in tree.import_list:
    filename = parsed_imp.import_filename
    print("imported file " + filename)
    import_tree = _UnpickleAST(os.path.join(ast_root_dir, os.path.splitext(os.path.basename(parsed_imp.import_filename))[0] + '.p'))
    imports[filename] = _GenerateModule(import_tree, filename)
  # Set the module path as relative to the source root.
  # Normalize to unix-style path here to keep the generators simpler.
  # module_path = rel_filename.relative_path().replace('\\', '/')
  path = os.path.basename(module_path)
  return translate.OrderedModule(tree, path, imports)

def parse_args():
  parser = argparse.ArgumentParser(description='Generate Obj-C files from mojo definitions')
  parser.add_argument('--pickled-ast', nargs=1)
  parser.add_argument('--module-include-path', nargs=1)
  parser.add_argument('--mojom-file', nargs=1)
  parser.add_argument('--output-dir', nargs=1)
  return parser.parse_args()

def main():
  global ast_root_dir

  args = parse_args()
  pickled_ast = args.pickled_ast[0]
  module_include_path = args.module_include_path[0]
  mojom_file = args.mojom_file[0]
  output_dir = args.output_dir[0]

  ast_root_dir = os.path.dirname(pickled_ast)

  tree = _UnpickleAST(pickled_ast)
  generator_module = importlib.import_module('mojom_objc_generator')
  bytecode_path = os.path.join(output_dir, "objc_templates_bytecode")
  fileutil.EnsureDirectoryExists(bytecode_path)
  template_expander.PrecompileTemplates({"objc": generator_module},
                                        bytecode_path)
  generator = generator_module.Generator(None)
  generator.bytecode_path = bytecode_path
  generator.module_include_path = module_include_path
  generator.module = _GenerateModule(tree, mojom_file)
  generator.GenerateFiles(output_dir)

if __name__ == "__main__":
  sys.exit(main())
