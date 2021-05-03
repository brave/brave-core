#!/usr/bin/env python3
# pylint:disable=line-too-long

"""
On Android the `--output' switch to `npm run test', which produces xunit
reports, doesn't work (https://github.com/brave/brave-browser/issues/15595).

This script reads json on stdin (`--json-results-file') and prints xunit xml
on stdout. An optional first command line argument will be used as a name for
the collection of test suites.

This is a temporary hack until `--output' on Android is fixed. Only one global
iteration is supported. In per-test iterations, only one is reported - the
first successful one, or the first failed one.
"""

import collections
import sys
import json
from operator import add
from functools import reduce

def transform(input_json):
    """Read json input and return a dictionary with information necessary to produce the xunit output"""

    output = collections.defaultdict(lambda: { 'xml': '', 'test_count': 0, 'failure_count': 0 })
    test_results = input_json['per_iteration_data'][0]
    for key, val in test_results.items():
        delim = '#' if '#' in key else '.'
        test_suite, test_case = key.rsplit(delim, maxsplit=1)
        output[test_suite]['test_count'] += 1
        output[test_suite]['xml'] += f'<testcase name="{test_case}" time="{int(val[0]["elapsed_time_ms"])/100.0}">'
        successful_iteration = next((iteration for iteration in val if iteration['status'] == 'SUCCESS'), None)
        if successful_iteration:
            if successful_iteration['output_snippet']:
                output[test_suite]['xml'] += f"<system-out><![CDATA[{successful_iteration['output_snippet']}]]></system-out>"
        else:
            output[test_suite]['failure_count'] += 1
            output[test_suite]['xml'] += f'<failure message="failed"><![CDATA[{val[0]["output_snippet"]}]]></failure>'
        output[test_suite]['xml'] += "</testcase>"
    return output

def main():
    """Read json on stdin and print xunit on stdout"""

    output = transform(json.load(sys.stdin))
    tss_name = "tests" if len(sys.argv) < 2 else sys.argv[1]
    test_count = reduce(add, (tc['test_count'] for tc in output.values()), 0)
    failure_count = reduce(add, (tc['failure_count'] for tc in output.values()), 0)
    print(f"""<?xml version="1.0" encoding="UTF-8"?>\n<testsuites name="{tss_name}" """
          f"""tests="{test_count}" errors="0" failures="{failure_count}" skip="0">""", end='')
    for test_suite in output.keys():
        print(f'<testsuite name="{test_suite}" tests="{output[test_suite]["test_count"]}" errors="0" failures='
              f'"{output[test_suite]["failure_count"]}" skip="0">{output[test_suite]["xml"]}</testsuite>', end='')
    print('</testsuites>', end='')

if __name__ == "__main__":
    main()
