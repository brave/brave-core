#!/usr/bin/env python3

# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This is a script that invokes googleTest executables in order to allow calls
# from a gn action.
# This script is meant to be called via
#   ./test_runner.py <gtest_exe> <out_dir> <...gtest_args>
# Which in turn executes:
#   gtest_exe \
#     -gtest_output=json:${out_dir}/results.json ...gtest_args \
#     > ${out_dir}/stdout.txt

# This script generates a depfile enabling ninja and
# gn to determine when to rerun the tests.
# It assumes that a <exe_path>.runtime_deps file exists.
# This file is generated for tests and any target that sets write_runtime_deps
# Note: all filepaths in a dep file need to be relative to build folder

import sys
import os
import subprocess
from datetime import datetime
import argparse
    
    
parser = argparse.ArgumentParser(
  description="Run google test suites with sharding for gn actions"
)

parser.add_argument("--shards", type=int, help="amount of shards the test suite should be split into")
parser.add_argument("--shardIndex", type=int, help="which test shard should be run")
parser.add_argument("--executable", type=str, help="which gtest executable shall be run")
parser.add_argument("--json", type=bool, default=False, help="generates test results xml or json if not set")
parser.add_argument("--outputDir", type=str, help="path to output folder; It will include stdout, stderr and result.{xml|json}")
parser.add_argument("--allow-failure", type=bool, default=False, help="will exit with success code even if some tests fail")
parser.add_argument("--filters", type=str, nargs="+", default=[], help="will exit with success code even if some tests fail")
# Use parse_known_args to avoid errors on unknown arguments



def main():
    (args, gtest_args) = parser.parse_known_args()
    print(datetime.today().strftime('%Y-%m-%d %H:%M:%S'))

    exe = args.executable
    out_dir = args.outputDir

    stdout_path = os.path.join(out_dir, "stdout.txt")
    result_path = os.path.join(out_dir, "result")

    reportFormat = "json" if (args.json) else "xml"
    gtest_output = f"--gtest_output={reportFormat}:{result_path}.{reportFormat}"

    print(f"running {exe}  SHARD {args.shardIndex} of {args.shards}")

    filterArgs = []
    if (len(args.filters)):
        filterArgs = ["--test-launcher-filter-file", ";".join(args.filters)]    

    try:
        envs = os.environ
        envs["GTEST_TOTAL_SHARDS"] = str(args.shards)
        envs["GTEST_SHARD_INDEX"] = str(args.shardIndex)
        process = subprocess.Popen([exe, gtest_output] + filterArgs + gtest_args,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   text=True,
                                   env=envs)

    except Exception as e:
        print(f"Failed to start process: {e}")
        sys.exit(1)

    with open(stdout_path, "w") as output_file:
        while True:
            stdout_line = process.stdout.readline()
            stderr_line = process.stderr.readline()

            if not stdout_line and not stderr_line and process.poll(
            ) is not None:
                break

            if stdout_line:
                sys.stdout.write(stdout_line)
                output_file.write(stdout_line)

            if stderr_line:
                sys.stderr.write(stderr_line)
                output_file.write(stderr_line)

    exit_code = process.wait()

    print(f"{exe} exited {exit_code} shard {args.shardIndex} / {args.shards}", sys.argv)
    sys.exit(0 if(args.allow_failure) else exit_code)


if __name__ == "__main__":
    main()
