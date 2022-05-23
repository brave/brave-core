#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import dsl_schema
import unittest


class DSLSchemaTest(unittest.TestCase):
  def testNamespaces(self):
    schema = dsl_schema.Load('test/namespaces.dsl')
    expected = [{
        "name":
        "base",
        "description":
        " This is the base namespace, appearing first in the file. It should be the first to be processed.",
        "type":
        "namespace",
        "types": [{
            "name": "internal",
            "description": " An internal namespace too",
            "type": "namespace",
            "types": []
        }]
    }, {
        "name": "net",
        "description": " Straggling net namespace",
        "type": "namespace",
        "types": []
    }]
    self.assertEqual(expected, schema)

  def testBasicEnum(self):
    schema = dsl_schema.Load('test/basic_enums.dsl')
    expected = [{
        "name":
        "base",
        "description":
        "",
        "type":
        "namespace",
        "types": [{
            "name":
            "Keys",
            "description":
            " A enum with simple key values",
            "type":
            "enum",
            "enum": [{
                "name": "kFirst",
                "value": "1",
                "description": "kFirst"
            }, {
                "name": "kSecond",
                "description": "kSecond"
            }, {
                "name": "kThird",
                "description": "kThird"
            }]
        }, {
            "name":
            "Colours",
            "description":
            " A enum with colours",
            "type":
            "enum",
            "enum": [{
                "name": "kBlue",
                "description": "kBlue"
            }, {
                "name": "kRed",
                "description": "kRed"
            }, {
                "name": "kMaxValue",
                "value": "kSecondName",
                "replacement": True
            }],
            "before":
            "kMaxValue"
        }]
    }]
    self.assertEqual(expected, schema)


  def testBasicClass(self):
    schema = dsl_schema.Load('test/classes.dsl')
    empty_class = {
        "name": "EmptyClass",
        "description": " Some empty class",
        "type": "class"
    }
    self.assertEqual(empty_class, schema[0].get("types")[0])

    # Testing member functions for SimpleClass

if __name__ == '__main__':
  unittest.main()
