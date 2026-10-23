import os
import sys

print("SAFE_POC_REVIEWDOG=" + str(bool(os.environ.get("REVIEWDOG_GITHUB_API_TOKEN"))))
sys.exit("SAFE_POC_STOP")
