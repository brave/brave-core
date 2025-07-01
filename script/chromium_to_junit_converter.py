#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Converts Chromium test results to JUnit XML format.

This script converts test results from Chromium's standard JSON format or
from ResultCollection objects to JUnit XML format for integration with
CI/CD systems that expect JUnit XML output.
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
            # TODO(bridiver) ?
            # is_unexpected = test_data.get('is_unexpected', False)

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

    def convert_from_result_collection(self, result_collection) -> str:
        """Converts a ResultCollection object to JUnit XML.

        Args:
            result_collection: ResultCollection object from test_result_util

        Returns:
            String containing JUnit XML
        """
        test_results = result_collection.test_results

        # Calculate statistics
        total_tests = len(test_results)
        failures = len([r for r in test_results if r.status == 'FAIL'])
        errors = len(
            [r for r in test_results if r.status in ['CRASH', 'ABORT']])
        skipped = len([r for r in test_results if r.status == 'SKIP'])

        # Calculate total duration
        total_duration = 0
        for test_result in test_results:
            if test_result.duration:
                total_duration += test_result.duration
        total_duration_seconds = total_duration / 1000.0  # ms to seconds

        # Create XML structure
        testsuite = ET.Element('testsuite')
        testsuite.set('name', 'ChromiumTests')
        testsuite.set('tests', str(total_tests))
        testsuite.set('failures', str(failures))
        testsuite.set('errors', str(errors))
        testsuite.set('skipped', str(skipped))
        testsuite.set('time', f'{total_duration_seconds:.3f}')
        testsuite.set('timestamp', time.strftime('%Y-%m-%dT%H:%M:%S'))

        if result_collection.crashed:
            testsuite.set('interrupted', 'true')

        # Add individual test cases
        for test_result in test_results:
            testcase = ET.SubElement(testsuite, 'testcase')
            testcase.set('name', test_result.name)
            testcase.set('classname',
                         self._extract_classname(test_result.name))

            # Add duration if available
            if test_result.duration:
                testcase.set('time', f'{test_result.duration / 1000.0:.3f}')

            # Add status-specific elements
            if test_result.status == 'FAIL':
                failure = ET.SubElement(testcase, 'failure')
                failure.set(
                    'message',
                    f'Test failed (expected: {test_result.expected_status})')
                if test_result.test_log:
                    failure.text = test_result.test_log
                if test_result.asan_failure_detected:
                    failure.set('type', 'ASan')
            elif test_result.status in ['CRASH', 'ABORT']:
                error = ET.SubElement(testcase, 'error')
                error.set('type', test_result.status)
                error.set(
                    'message',
                    f'Test {test_result.status.lower()} (expected: {test_result.expected_status})'
                )
                if test_result.test_log:
                    error.text = test_result.test_log
            elif test_result.status == 'SKIP':
                skipped_elem = ET.SubElement(testcase, 'skipped')
                skipped_elem.set(
                    'message',
                    f'Test skipped (expected: {test_result.expected_status})')
                if test_result.disabled():
                    skipped_elem.set('type', 'disabled')

        # Add crash message if collection crashed
        if result_collection.crashed and result_collection.crash_message:
            system_out = ET.SubElement(testsuite, 'system-out')
            system_out.text = result_collection.crash_message

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
        description='Convert Chromium test results to JUnit XML format')
    parser.add_argument(
        'input_file', help='Input JSON file containing Chromium test results')
    parser.add_argument('-o',
                        '--output',
                        help='Output XML file (default: stdout)')
    parser.add_argument('--format',
                        choices=['json', 'collection'],
                        default='json',
                        help='Input format (default: json)')

    args = parser.parse_args()

    # Read input file
    try:
        with open(args.input_file, 'r') as f:
            if args.format == 'json':
                data = json.load(f)
            else:
                # For collection format, would need to import and load the
                # ResultCollection
                print("Collection format not supported via file input")
                return 1
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error reading input file: {e}", file=sys.stderr)
        return 1

    # Convert to JUnit XML
    converter = ChromiumToJUnitConverter()
    try:
        if args.format == 'json':
            xml_output = converter.convert_from_json(data)
        else:
            # Would handle ResultCollection here if supported
            xml_output = ""
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
