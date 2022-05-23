#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# This file provides a parser for a patching DSL schema, which can be used for
# patching C++ files, by using an IDL parser to interprest the patching DLS and
# returning a JSON object describing it.

from __future__ import print_function

import itertools
import json
import os.path
import pprint
import re
import sys

if sys.version_info.major == 2:
  from itertools import izip_longest as zip_longest
else:
  from itertools import zip_longest

from collections import OrderedDict

# idl_parser expects to be able to import certain files in its directory,
# so let's set things up the way it wants.
_idl_generators_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                    'parser')
if _idl_generators_path in sys.path:
  import idl_parser
else:
  sys.path.insert(0, _idl_generators_path)
  try:
    import idl_parser
  finally:
    sys.path.pop(0)


class Enum(object):
  '''
  Given a DSL Enum node, converts into a Python dictionary that the patching
  engine expects to see.
  '''

  def __init__(self, enum_node):
    self.node = enum_node
    self.description = ''

  def process(self):
    enum = []
    for node in self.node.GetChildren():
      if node.cls == 'EnumItem':
        enum_value = {'name': node.GetName()}
        value = node.GetProperty('VALUE')
        if value is not None:
          enum_value['value'] = value
        if node.GetProperty('replacement'):
          enum_value['replacement'] = True
        for child in node.GetChildren():
          if child.cls == 'Comment':
            enum_value['description'] = node.GetName().replace('\n', '')
          else:
            raise ValueError('Did not process %s %s' % (child.cls, child))
        enum.append(enum_value)
      elif node.cls == 'Comment':
        self.description = node.GetName().replace('\n', '')
      else:
        sys.exit('Did not process %s %s' % (node.cls, node))
    result = {
        'name': self.node.GetName(),
        'description': self.description,
        'type': 'enum',
        'enum': enum
    }
    for property_name in ['before']:
      if self.node.GetProperty(property_name):
        result[property_name] = self.node.GetProperty(property_name)
    return result


class Callspec(object):
  '''
  Given a Callspec node representing a DSL function declaration, converts into
  a tuple:
      (name, list of function parameters, return type, async return)
  '''

  def __init__(self, callspec_node):
    self.node = callspec_node

  def process(self):
    parameters = []
    result = {
        'name': self.node.GetName(),
        'type': 'function',
        'return': self.node.GetProperty('TYPEREF'),
    }

    for node in self.node.GetChildren():
      parameters.append({
          'name': node.GetName(),
          'type': node.GetProperty('TYPEREF')
      })
    result['params'] = parameters

    if self.node.GetProperty('STATIC') is True:
      result['static'] = True

    return result


class Member(object):
  '''
  Given an IDL class, provide a JSON description of data members and fucntions
  that the patching engine expects to see.
  '''

  def __init__(self, member_node):
    self.node = member_node
    self.description = ''

  def process(self):
    result = {}
    for node in self.node.GetChildren():
      if node.cls == 'Comment':
        self.description = node.GetName().replace('\n', '')
      elif node.cls == 'Callspec':
        result = Callspec(node).process()
      else:
        sys.exit('Did not process %s %s' % (node.cls, node))
    return result | {'description': self.description}


class Class(object):
  '''
  Given a DSL Class node, converts into a Python dictionary that the patching
  engine expects to see.
  '''

  def __init__(self, enum_node):
    self.node = enum_node
    self.description = ''

  def process(self):
    members = []
    for node in self.node.GetChildren():
      if node.cls == 'Comment':
        self.description = node.GetName().replace('\n', '')
      elif node.cls == 'Member':
        members.append(Member(node).process())
      else:
        sys.exit('Did not process %s %s' % (node.cls, node))

    return {
        'name': self.node.GetName(),
        'description': self.description,
        'type': 'class',
        'members': members
    }


class Namespace(object):
  '''
  Given an IDLNode representing a DSL namespace, converts into a Python
  dictionary that the patching schema compiler expects to see.
  '''

  def __init__(self, namespace_node, description=''):
    self.namespace = namespace_node
    self.types = []
    self.description = description

  def process(self):
    for node in self.namespace.GetChildren():
      if node.cls == 'Namespace':
        self.types.append(Namespace(node).process())
      elif node.cls == 'Class':
        self.types.append(Class(node).process())
      elif node.cls == 'Enum':
        self.types.append(Enum(node).process())
      elif node.cls == 'Comment':
        self.description = node.GetName().replace('\n', '')
      else:
        sys.exit('Did not process %s %s' % (node.cls, node))

    return {
        'name': self.namespace.GetName(),
        'description': self.description,
        'type': 'namespace',
        'types': self.types
    }


class DSLSchema(object):
  '''
  Given a list of IDLNodes and IDLAttributes, converts into a Python list
  of api_defs that the patching engine expects to see.
  '''

  def __init__(self, idl):
    self.idl = idl

  def process(self):
    contents = []
    description = ''

    for node in self.idl:
      if node.cls == 'Namespace':
        namespace = Namespace(node, description)
        contents.append(namespace.process())
        continue
      elif node.cls == 'Copyright':
        continue
      if node.cls == 'Comment':
        description = node.GetName().replace('\n', '')
        continue
      else:
        sys.exit('Did not process %s %s' % (node.cls, node))

    return contents


def Load(filename):
  '''
  Given the filename of patch file, parses it and returns an equivalent
  Python dictionary in a format that the patching engine expects to see.
  '''

  with open(filename, 'rb') as handle:
    contents = handle.read().decode('utf-8')

  return Process(contents, filename)


def Process(contents, filename):
  '''
  Processes the contents of a file and returns an equivalent Python dictionary
  in a format that the patching engine expects to see. (Separate from Load
  primarily for testing purposes.)
  '''

  idl = idl_parser.IDLParser().ParseData(contents, filename)
  pacthing_schema = DSLSchema(idl)
  return pacthing_schema.process()


def Main():
  '''
  Dump a json serialisation of parse result for the patch files whose names
  were passed in on the command line.
  '''
  if len(sys.argv) > 1:
    for filename in sys.argv[1:]:
      schema = Load(filename)
      print(json.dumps(schema, indent=2))
  else:
    contents = sys.stdin.read()
    for i, char in enumerate(contents):
      if not char.isascii():
        raise Exception(
            'Non-ascii character "%s" (ord %d) found at offset %d.' %
            (char, ord(char), i))
    idl = idl_parser.IDLParser().ParseData(contents, '<stdin>')
    schema = IDLSchema(idl).process()
    print(json.dumps(schema, indent=2))


if __name__ == '__main__':
  Main()
