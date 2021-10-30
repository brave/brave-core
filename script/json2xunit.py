#!/usr/bin/env python3
# pylint:disable=line-too-long,consider-using-dict-items

"""
On Android the `--output' switch to `npm run test', which produces xunit
reports, doesn't work (https://github.com/brave/brave-browser/issues/15595).

This script reads json on stdin (`--json-results-file') and prints xunit xml
on stdout.

This is a temporary hack until `--output' on Android is fixed. Only one global
test iteration is supported (it seems only one is used anyway). In per-test
iterations, only one is reported - a successful one, if it exists, otherwise a
failed one.
"""

import collections
import re
import sys
import json
from operator import add
from string import printable
from functools import reduce


def pick_iteration(test_case, iterations):
    """Pick the test iteration that provides the most relevant feedback"""

    def score(iteration):
        score = 1000000 if iteration['status'] == 'SUCCESS' else 0
        score += len(iteration['output_snippet'])
        started_this_test = re.compile(
            r'\[ RUN      \] [\w/#\.]*' + re.escape(test_case) + '\\n')
        score += 7500 if started_this_test.match(
            iteration['output_snippet']) else 0
        for string in ['Stack Trace:', 'Expected:', 'Actual:']:
            score += 2500 if string in iteration['output_snippet'] else 0
        return score
    return sorted(iterations, key=score, reverse=True)[0]


def transform(input_json):
    """Read json input and return a dictionary with information necessary to produce the xunit output"""

    output = collections.defaultdict(
        lambda: {'xml': '', 'test_count': 0, 'failure_count': 0})
    test_results = input_json['per_iteration_data'][0]
    for test_fullname, iterations in test_results.items():
        delim = '#' if '#' in test_fullname else '/' if '/' in test_fullname else '.'
        test_suite, test_case = test_fullname.rsplit(delim, maxsplit=1)
        iteration = pick_iteration(test_case, iterations)

        output[test_suite]['test_count'] += 1
        output[test_suite][
            'xml'] += f'<testcase name="{test_case}" time="{int(iteration["elapsed_time_ms"])/100.0}">'
        if iteration['status'] == 'SUCCESS':
            if iteration['output_snippet']:
                sanitized_output = ''.join(
                    filter(lambda x: x in printable, iteration['output_snippet']))
                output[test_suite]['xml'] += f"<system-out><![CDATA[{sanitized_output}]]></system-out>"
        else:
            output[test_suite]['failure_count'] += 1
            sanitized_output = ''.join(
                filter(lambda x: x in printable, iteration['output_snippet']))
            output[test_suite][
                'xml'] += f'<failure message="failed"><![CDATA[{sanitized_output}]]></failure>'
        output[test_suite]['xml'] += "</testcase>"
    return output


def main():
    """Read json on stdin and print xunit on stdout"""

    output = transform(json.load(sys.stdin))
    test_count = reduce(add, (ts['test_count'] for ts in output.values()), 0)
    failure_count = reduce(add, (ts['failure_count']
                           for ts in output.values()), 0)

    print(f"""<?xml version="1.0" encoding="UTF-8"?>\n<testsuites name="tests" """
          f"""tests="{test_count}" errors="0" failures="{failure_count}" skip="0">""", end='')
    for test_suite in output.keys():
        print(f'<testsuite name="{test_suite}" tests="{output[test_suite]["test_count"]}" errors="0" failures='
              f'"{output[test_suite]["failure_count"]}" skip="0">{output[test_suite]["xml"]}</testsuite>', end='')
    print('</testsuites>', end='')


if __name__ == "__main__":
    main()
