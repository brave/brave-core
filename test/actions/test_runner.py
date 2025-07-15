#!/usr/bin/env python3
# This is a script that invokes googleTest executables in order to allow calls from a gn action
# This script is meant to be called via ./test_runner.py <gtest_exe> <out_dir> <...gtest_args>
# Which in turn executes: 
#   gtest_exe -gtest_output=json:${out_dir}/results.json ...gtest_args > ${out_dir}/stdout.txt
# stdout.txt acts as a cache and the script will print stdout.txt if it's newer than gtest_exe

# This script generates a depfile enabling ninja and gn to determine when to rerun the tests.
# It assumes that a <exe_path>.runtime_deps file exists. 
# This file is present test targets or if you explicitly set write_runtime_deps = <path>
# Note: all filepaths in a dep file need to be relative to build folder as your local absolute path might not exist on RBE

import sys
import os
import subprocess
from datetime import datetime

def main():
  print(datetime.today().strftime('%Y-%m-%d %H:%M:%S'))
  if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <gtest_exe> <out_dir> [args...]")
    sys.exit(1)

  exe = sys.argv[1]
  out_dir = sys.argv[2]
  args = sys.argv[3:]


  print(' '.join(sys.argv[1:]))

  input_files = [x for x in args if not x.startswith('-')]
  other_args = [x for x in args if x.startswith('-')]

  stdout_path = os.path.join(out_dir, "stdout.txt")
  result_path = os.path.join(out_dir, "result.json")

  gtest_output = f"--gtest_output=json:{result_path}"

  try:
    process = subprocess.Popen(
      [exe, gtest_output] + other_args,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )

  except Exception as e:
    print(f"Failed to start process: {e}")
    sys.exit(1)

  with open(stdout_path, "w") as output_file:
    while True:
      stdout_line = process.stdout.readline()
      stderr_line = process.stderr.readline()

      if not stdout_line and not stderr_line and process.poll() is not None:
        break

      if stdout_line:
        sys.stdout.write(stdout_line)
        output_file.write(stdout_line)

      if stderr_line:
        sys.stderr.write(stderr_line)
        output_file.write(stderr_line)

  exit_code = process.wait()
  sys.exit(exit_code)

if __name__ == "__main__":
  main()
