# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
"""Patches mojo modules using *.mojom files from 'chromium_src' dir."""

import codecs
import os.path

from mojom.parse import ast
from mojom.parse import conditional_features
from mojom.parse import parser


# Attribute constant to mark a definition as a new one.
# If a definition with same name already exists, it's an error to add a new one.
_DEFINITION_ADD = 1
# Attribute constant to mark a definition as the one to extend.
# If a definition with same name and type doesn't exist, it's an error to extend one.
_DEFINITION_EXTEND = 2


# Predicate for an import filename.
def _AstImportPred(brave_import, ast_import):
  return brave_import.import_filename == ast_import.import_filename


# Predicate for a definition name (doesn't check type).
def _AstDefinitionNamePred(brave_definition, ast_definition):
  return brave_definition.mojom_name == ast_definition.mojom_name


# Predicate for definition name and type.
def _AstDefinitionPred(brave_definition, ast_definition):
  return _AstDefinitionNamePred(brave_definition, ast_definition) and \
            isinstance(brave_definition, type(ast_definition))


# Returns required action from a definition attributes list.
def _GetBraveDefinitionAction(brave_definition):
  if isinstance(brave_definition, ast.Const):
    # Const can only be added.
    return _DEFINITION_ADD
  if brave_definition.attribute_list:
    for attribute in brave_definition.attribute_list:
      if attribute.key == 'BraveAdd':
        return _DEFINITION_ADD
      elif attribute.key == 'BraveExtend':
        return _DEFINITION_EXTEND
  raise ValueError("Definition should have [BraveAdd] or [BraveExtend] attribute: %s" %
                   brave_definition.mojom_name)


# Validates that a definition with same name doesn't exist in the list.
def _CheckDefinitionDoesntExist(brave_definition, ast_items):
  if any(_AstDefinitionNamePred(brave_definition, item) for item in ast_items):
    raise ValueError("Definition already exists: %s" % brave_definition.mojom_name)


# Finds a definition by name and type.
def _FindMatchingDefinition(brave_definition, ast_definitions):
  for ast_definition in ast_definitions:
    if _AstDefinitionPred(brave_definition, ast_definition):
      return ast_definition


# Adds new items to an existing mojom definition.
def _ExtendAstDefinition(brave_definition, ast_definition):
  if isinstance(brave_definition, ast.Enum):
    for item in brave_definition.enum_value_list:
      _CheckDefinitionDoesntExist(item, ast_definition.enum_value_list)
      ast_definition.enum_value_list.Append(item)
  elif isinstance(brave_definition, ast.Interface) or \
       isinstance(brave_definition, ast.Struct) or \
       isinstance(brave_definition, ast.Union):
    items_to_append = []
    for item in reversed(brave_definition.body.items):
      if isinstance(item, ast.Const) or \
         isinstance(item, ast.Enum):
         # Handle nested types.
        _ApplyBraveDefinition(item, ast_definition.body)
      else:
        _CheckDefinitionDoesntExist(item, ast_definition.body)
        items_to_append.append(item)
    # Restore members order and append them as declared.
    for item in reversed(items_to_append):
      ast_definition.body.Append(item)
  else:
    raise ValueError("Unhandled definition: %s" % brave_definition.mojom_name)


# Adds or extends mojom ast definition.
def _ApplyBraveDefinition(brave_definition, ast_definitions):
  definition_action = _GetBraveDefinitionAction(brave_definition)
  if definition_action == _DEFINITION_ADD:
    _CheckDefinitionDoesntExist(brave_definition, ast_definitions)
    if isinstance(ast_definitions, ast.NodeListBase):
      ast_definitions.Insert(brave_definition)
    else:
      ast_definitions.insert(0, brave_definition)
  elif definition_action == _DEFINITION_EXTEND:
    ast_definition_to_extend = _FindMatchingDefinition(brave_definition, ast_definitions)
    if not ast_definition_to_extend:
      raise ValueError("Trying to extend non-existent definition: %s" % brave_definition.mojom_name)
    _ExtendAstDefinition(brave_definition, ast_definition_to_extend)
  else:
    raise ValueError("Unknown definition action requested: %s" % brave_definition.mojom_name)


# Applies changes to original mojom ast using brave ast.
def _ApplyBraveAstChanges(brave_ast, ast):
  # Make sure the module name is correct.
  if brave_ast.module != ast.module:
    raise ValueError("Mojo module ids are not equal while trying to patch: %s vs %s" % \
          (brave_ast.module.mojom_namespace, ast.module.mojom_namespace))

  # Add new imports.
  for brave_import in brave_ast.import_list:
    if not any(_AstImportPred(brave_import, imp) for imp in ast.import_list):
      ast.import_list.Append(brave_import)

  # Add/extend mojo definitions and keep the type dependency order valid.
  #
  # At a later stage a mojo generator expects all types to be in order of use.
  # To acknowledge this we insert all new definitions in reversed order at
  # 0-position which will effectively place them in the right order as it was
  # declared in chromium_src/**/*.mojom *before* all existing definitions in the
  # original mojom.
  #
  # Enum values, struct/union members, interface methods are always appended.
  for brave_definition in reversed(brave_ast.definition_list):
    _ApplyBraveDefinition(brave_definition, ast.definition_list)


def PatchMojomAst(mojom_abspath, ast, enabled_features):
  """Search for mojom file in 'chromium_src' and apply AST changes.

  Can add imports, enums, interfaces, structs, unions.
  Can extend enum values, interface members, struct fields, union members.

  To be more strict with patching, attributes [BraveAdd] or [BraveExtend] should be used.
  """

  # Get this script absolute location.
  this_py_path = os.path.realpath(__file__)

  # Get the original chromium dir location.
  chromium_original_dir = os.path.abspath(os.path.join(this_py_path,
                                                       *[os.pardir] * 10))

  if len(chromium_original_dir) >= len(mojom_abspath) + 1:
    raise RuntimeError("Could not get original chromium src dir")

  # Build brave/chromium_src path.
  chromium_src_abspath = os.path.join(chromium_original_dir, 'brave', 'chromium_src')
  if not os.path.isdir(chromium_src_abspath):
    raise RuntimeError("Could not find brave/chromium_src. %s is not a dir" % chromium_src_abspath)

  # Relative path.
  mojom_relpath = mojom_abspath[len(chromium_original_dir) + 1:]

  # Build possible brave/chromium_src/**/*.mojom path.
  brave_mojom_abspath = os.path.join(chromium_src_abspath, mojom_relpath)
  if not os.path.isfile(brave_mojom_abspath):
    # Nothing to patch.
    return

  # Open and parse brave/chromium_src/**/*.mojom file.
  with codecs.open(brave_mojom_abspath, encoding='utf-8') as f:
    brave_ast = parser.Parse(f.read(), brave_mojom_abspath)
    conditional_features.RemoveDisabledDefinitions(brave_ast, enabled_features)

    _ApplyBraveAstChanges(brave_ast, ast)
