#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
import tempfile
import os
import json

from env_config import read_env_config_as_dict


class TestReadEnvConfigAsDict(unittest.TestCase):

    def setUp(self):
        # Create a temporary directory to store test files
        self.test_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        # Clean up the temporary directory
        self.test_dir.cleanup()

    def create_temp_file(self, content, filename="test.env", encoding="utf-8"):
        # Helper function to create a temporary file with the given content
        file_path = os.path.join(self.test_dir.name, filename)
        with open(file_path, 'w', encoding=encoding) as f:
            f.write(content)
        return file_path

    def assert_env_config_value(self, result, key, value):
        self.assertEqual(result[key], value)
        self.assertEqual(result[key + '_escaped'], json.dumps(value))

    def test_env_config(self):
        content = """
        SAMPLE_KEY=sample_value
        ANOTHER_KEY="another value"
        ONE_MORE_KEY = test
        UNQUOTED_VAL_WITH_SPACES = value with spaces
        """
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'SAMPLE_KEY', 'sample_value')
        self.assert_env_config_value(result, 'ANOTHER_KEY', 'another value')
        self.assert_env_config_value(result, 'ONE_MORE_KEY', 'test')
        self.assert_env_config_value(result, 'UNQUOTED_VAL_WITH_SPACES',
                                     'value with spaces')

    def test_key_is_uppercased(self):
        content = "sample_key=sample_value"
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'sample_key', 'sample_value')
        self.assert_env_config_value(result, 'SAMPLE_KEY', 'sample_value')

    def test_quoted_strings(self):
        for quote in ['`', '"', "'"]:
            content = f"SAMPLE_KEY={quote}sample_value{quote}"
            file_path = self.create_temp_file(content)
            result = read_env_config_as_dict(file_path)
            self.assert_env_config_value(result, 'SAMPLE_KEY', 'sample_value')

    def test_comments(self):
        content = """
        # A comment
        SAMPLE_KEY=sample_value # another comment
        """
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'SAMPLE_KEY', 'sample_value')

    def test_include_env(self):
        main_content = """
        VAR1=val1
        VAR2=val2
        include_env=include.env
        """
        include_content = """
        INCLUDED_KEY=included_value
        VAR2=val2_override
        """
        main_file_path = self.create_temp_file(main_content, "main.env")
        include_file_path = self.create_temp_file(include_content,
                                                  "include.env")
        result = read_env_config_as_dict(main_file_path)
        self.assert_env_config_value(result, 'VAR1', 'val1')
        self.assert_env_config_value(result, 'VAR2', 'val2_override')
        self.assert_env_config_value(result, 'INCLUDED_KEY', 'included_value')
        self.assertIn(include_file_path.replace("\\", "/"),
                      result['include_env'])

    def test_json_value(self):
        content = 'JSON_KEY={"key": "value"}'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'JSON_KEY', {"key": "value"})

    def test_bool_value(self):
        content = 'BOOL_KEY=true'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'BOOL_KEY', True)

    def test_number_value(self):
        content = 'NUMBER_KEY=10'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'NUMBER_KEY', 10)

    def test_invalid_json_value(self):
        content = 'INVALID_JSON_KEY={key: value}'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'INVALID_JSON_KEY',
                                     '{key: value}')

    def test_multiline_string(self):
        content = 'MULTILINE_KEY="line1\\nline2"'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'MULTILINE_KEY', 'line1\nline2')

    def test_multiline_string2(self):
        content = 'MULTILINE_KEY="line1\nline2"'
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'MULTILINE_KEY', 'line1\nline2')

    def test_invalid_keys(self):
        content = """
        1key=sample_value
        key-two=sample_value
        key.three=sample_value
        _KEY_VALID=valid_key
        """
        file_path = self.create_temp_file(content)
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, '_KEY_VALID', 'valid_key')
        # Expect only _KEY_VALID and _KEY_VALID_escaped in the result.
        self.assertEqual(len(result), 2)

    def test_utf_8_with_bom(self):
        content = "SAMPLE_KEY=sample_value"
        file_path = self.create_temp_file(content, encoding="utf-8-sig")
        result = read_env_config_as_dict(file_path)
        self.assert_env_config_value(result, 'SAMPLE_KEY', 'sample_value')


if __name__ == '__main__':
    unittest.main()
