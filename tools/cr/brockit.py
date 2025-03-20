#!/usr/bin/env vpython3
import os
import subprocess
import sys

env = os.environ.copy()
env["GIT_CACHE_PATH"] = "/home/ubuntu/cache"

def run_npm_init():
    process = subprocess.Popen(
        ["npm run init"],
        stdout=sys.stdout,
        stderr=sys.stderr,
        shell=True,
        env=env,
    )
    process.wait()

if __name__ == "__main__":
    run_npm_init()