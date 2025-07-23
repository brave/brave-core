#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Converts Chromium test results to JUnit XML format.

This script converts test results from Chromium's standard JSON format to JUnit
XML format for integration with CI/CD systems that expect JUnit XML output.
"""

import argparse
import json
import sys
import time
import xml.etree.ElementTree as ET
from xml.dom import minidom
from typing import Dict, List, Optional, Union


class ChromiumToJUnitConverter:
    """Converts Chromium test results to JUnit XML format."""

    def __init__(self):
        self.status_mapping = {
            'PASS': 'passed',
            'FAIL': 'failure',
            'CRASH': 'error',
            'TIMEOUT': 'error',
            'ABORT': 'error',
            'SKIP': 'skipped'
        }

    def convert_from_summary_json(self, json_data: Dict) -> str:
        """Converts iOS unit test runner summary.json to JUnit XML.

        Args:
            json_data: Dictionary containing iOS unit test runner summary.json

        Returns:
            String containing JUnit XML
        """
        return self._convert_ios_summary(json_data)

    def _convert_ios_summary(self, json_data: Dict) -> str:
        """Converts iOS unit test runner summary.json format to JUnit XML."""
        logs = json_data.get('logs', {})
        passed_tests = logs.get('passed tests', [])
        failed_tests = logs.get('failed tests', {})
        step_text = json_data.get('step_text', '')

        # Calculate totals
        total_passed = len(passed_tests)
        total_failed = len(failed_tests)
        has_execution_error = bool(step_text
                                   and 'TEST EXECUTE FAILED' in step_text)
        total_tests = total_passed + total_failed + (1 if has_execution_error
                                                     else 0)

        # Create XML structure
        testsuite = ET.Element('testsuite')
        testsuite.set('name', 'IOSTests')
        testsuite.set('tests', str(total_tests))
        testsuite.set('failures', str(total_failed))
        testsuite.set('errors', '1' if has_execution_error else '0')
        testsuite.set('skipped', '0')
        testsuite.set('time', '0')  # iOS runner doesn't provide timing info
        testsuite.set('timestamp', time.strftime('%Y-%m-%dT%H:%M:%S'))

        # Add passed tests
        for test_name in passed_tests:
            testcase = ET.SubElement(testsuite, 'testcase')
            testcase.set('name', test_name)
            testcase.set('classname', self._extract_classname(test_name))

        # Add failed tests
        for test_name, failure_logs in failed_tests.items():
            testcase = ET.SubElement(testsuite, 'testcase')
            testcase.set('name', test_name)
            testcase.set('classname', self._extract_classname(test_name))

            failure = ET.SubElement(testcase, 'failure')
            failure.set('message', f'Test {test_name} failed')

            # Add failure details from logs
            if failure_logs:
                failure_text = []
                # pylint: disable=unused-variable
                for i, log_entry in enumerate(failure_logs):
                    if isinstance(log_entry, str):
                        failure_text.append(log_entry)
                    else:
                        failure_text.append(str(log_entry))
                failure.text = '\n'.join(failure_text)

        # Add execution error if present
        if has_execution_error:
            testcase = ET.SubElement(testsuite, 'testcase')
            testcase.set('name', 'TestExecution')
            testcase.set('classname', 'TestExecution')

            error = ET.SubElement(testcase, 'error')
            error.set('type', 'ExecutionError')
            error.set('message', 'Test execution failed')
            error.text = step_text

        return self._prettify_xml(testsuite)

    def convert_from_json(self, json_data: Dict) -> str:
        """Converts Chromium standard JSON test results to JUnit XML.

        Args:
            json_data: Dictionary containing Chromium test results in standard
            format

        Returns:
            String containing JUnit XML
        """
        # Extract test results
        tests = json_data.get('tests', {})
        num_failures_by_type = json_data.get('num_failures_by_type', {})
        interrupted = json_data.get('interrupted', False)

        # Calculate totals
        total_tests = sum(num_failures_by_type.values())
        failures = num_failures_by_type.get('FAIL', 0)
        errors = (num_failures_by_type.get('CRASH', 0) +
                  num_failures_by_type.get('TIMEOUT', 0) +
                  num_failures_by_type.get('ABORT', 0))
        skipped = num_failures_by_type.get('SKIP', 0)

        # Create XML structure
        testsuite = ET.Element('testsuite')
        testsuite.set('name', 'ChromiumTests')
        testsuite.set('tests', str(total_tests))
        testsuite.set('failures', str(failures))
        testsuite.set('errors', str(errors))
        testsuite.set('skipped', str(skipped))
        testsuite.set('time', '0')  # Duration not available in standard format
        testsuite.set('timestamp', time.strftime('%Y-%m-%dT%H:%M:%S'))

        if interrupted:
            testsuite.set('interrupted', 'true')

        # Add individual test cases
        for test_name, test_data in tests.items():
            testcase = ET.SubElement(testsuite, 'testcase')
            testcase.set('name', test_name)
            testcase.set('classname', self._extract_classname(test_name))

            # Handle actual results (may be multiple if test was retried)
            actual_results = test_data.get('actual', '').split()
            expected = test_data.get('expected', 'PASS')

            # Use the last result as the final result
            final_result = actual_results[-1] if actual_results else 'UNKNOWN'

            if final_result == 'FAIL':
                failure = ET.SubElement(testcase, 'failure')
                failure.set('message', f'Test failed (expected: {expected})')
                if len(actual_results) > 1:
                    failure.text = f'Test results: {" ".join(actual_results)}'
            elif final_result in ['CRASH', 'TIMEOUT', 'ABORT']:
                error = ET.SubElement(testcase, 'error')
                error.set('type', final_result)
                error.set(
                    'message',
                    f'Test {final_result.lower()} (expected: {expected})')
                if len(actual_results) > 1:
                    error.text = f'Test results: {" ".join(actual_results)}'
            elif final_result == 'SKIP':
                skipped_elem = ET.SubElement(testcase, 'skipped')
                skipped_elem.set('message',
                                 f'Test skipped (expected: {expected})')

            # Mark flaky tests
            if test_data.get('is_flaky', False):
                testcase.set('flaky', 'true')

        return self._prettify_xml(testsuite)

    def _extract_classname(self, test_name: str) -> str:
        """Extracts class name from a test name.

        Args:
            test_name: Full test name (e.g., 'com.example.TestClass.testMethod')

        Returns:
            Class name portion or the test name itself if no class can be
            extracted
        """
        # Handle common patterns:
        # - Java style: com.example.TestClass.testMethod
        # - C++ style: TestSuite.TestCase or TestSuite::TestCase
        if '.' in test_name:
            parts = test_name.split('.')
            if len(parts) >= 2:
                return '.'.join(parts[:-1])  # Everything except the last part
        elif '::' in test_name:
            parts = test_name.split('::')
            if len(parts) >= 2:
                return '::'.join(parts[:-1])  # Everything except the last part

        # Fallback: use the test name as class name
        return test_name

    def _prettify_xml(self, element: ET.Element) -> str:
        """Prettifies XML output.

        Args:
            element: XML element to prettify

        Returns:
            Pretty-printed XML string
        """
        rough_string = ET.tostring(element, encoding='unicode')
        reparsed = minidom.parseString(rough_string)
        return reparsed.toprettyxml(indent='  ').strip()


def main():
    """Main function for command-line usage."""
    parser = argparse.ArgumentParser(
        description='Convert Chromium/iOS test results to JUnit XML format')
    parser.add_argument('input_file',
                        help='Input JSON file containing test results')
    parser.add_argument('-o',
                        '--output',
                        help='Output XML file (default: stdout)')
    parser.add_argument('--format',
                        choices=['auto', 'standard', 'summary'],
                        default='auto',
                        help='Input file format (default: auto-detect)')
    args = parser.parse_args()

    # Read input file
    try:
        with open(args.input_file, 'r') as f:
            data = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error reading input file: {e}", file=sys.stderr)
        return 1

    # Convert to JUnit XML
    converter = ChromiumToJUnitConverter()
    try:
        if args.format == 'summary':
            xml_output = converter.convert_from_summary_json(data)
        elif args.format == 'standard':
            xml_output = converter.convert_from_json(data)
        else:  # auto-detect
            if args.input_file == 'summary.json':
                xml_output = converter.convert_from_summary_json(data)
            else:
                # Default to standard format
                xml_output = converter.convert_from_json(data)
    except Exception as e:
        print(f"Error converting test results: {e}", file=sys.stderr)
        return 1

    # Write output
    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(xml_output)
            print(f"JUnit XML written to {args.output}")
        except IOError as e:
            print(f"Error writing output file: {e}", file=sys.stderr)
            return 1
    else:
        print(xml_output)

    return 0


if __name__ == '__main__':
    sys.exit(main())
