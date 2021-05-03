#!/usr/bin/env python3
# On Android the `--output' switch to `npm run test', which produces xunit
# reports, doesn't work (https://github.com/brave/brave-browser/issues/15595).
#
# This script reads json on stdin (`--json-results-file') and prints xunit xml
# on stdout. An optional first command line argument will be used as a name for
# the collection of test suites.
#
# This is a temporary hack until `--output' on Android is fixed. Multiple test
# iterations, skipped tests and other features are not supported.

import collections, sys, json, operator
from functools import reduce

def transform(input):
    output = collections.defaultdict(lambda: { 'xml': '', 'test_count': 0, 'failure_count': 0 })
    test_results = input['per_iteration_data'][0]
    for k, v in test_results.items():
        delim = '#' if '#' in k else '.'
        test_suite, test_case = k.rsplit(delim, maxsplit=1)
        output[test_suite]['test_count'] += 1
        output[test_suite]['xml'] += f'<testcase name="{test_case}" time="{v[0]["elapsed_time_ms"]}">'
        if v[0]['status'] == 'SUCCESS':
            if v[0]['output_snippet']:
                output[test_suite]['xml'] += f"<system-out><![CDATA[{v[0]['output_snippet']}]]></system-out>"
        else:
            output[test_suite]['failure_count'] += 1
            output[test_suite]['xml'] += f'<failure message="failed"><![CDATA[{v[0]["output_snippet"]}]]></failure>'
        output[test_suite]['xml'] += "</testcase>"
    return output

def main():
    output = transform(json.load(sys.stdin))
    tss_name = "tests" if len(sys.argv) < 2 else sys.argv[1]
    test_count = reduce(operator.add, (tc['test_count'] for tc in output.values()), 0)
    failure_count = reduce(operator.add, (tc['failure_count'] for tc in output.values()), 0)
    print(f"""<?xml version="1.0" encoding="UTF-8"?>\n<testsuites name="{tss_name}" """
          f"""tests="{test_count}" errors="0" failures="{failure_count}" skip="0">""", end="")
    for test_suite in output.keys():
        print(f'<testsuite name="{test_suite}" tests="{output[test_suite]["test_count"]}" errors="0" '
              f'failures="{output[test_suite]["failure_count"]}" skip="0">{output[test_suite]["xml"]}</testsuite>', end="")
    print('</testsuites>', end="")

if __name__ == "__main__":
    main()
