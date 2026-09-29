# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import tempfile
import time
import optparse
import os
import shutil
import subprocess
import sys
import re
from urlparse import urlparse


def collect_data(browser, out_dir, url):
    result = re.search(r"(\d*)(.*)", url)

    ticket = ''

    if len(result.groups()) > 1:
        ticket = result.groups()[0]
        url = result.groups()[1]

    out = f"{out_dir}/{urlparse(url.strip()).netloc}"
    out = os.path.abspath(out)
    distilled_path = f"{out}/distilled.html"

    if os.path.exists(distilled_path):
        print(f"Skip {urlparse(url).netloc}")
        return 0

    temp_dir = tempfile.mkdtemp()
    time.sleep(1)

    print(f"Processing {url} to {out}")
    with subprocess.Popen([
            browser, url, f"--user-data-dir={temp_dir}",
            f"--speedreader-collect-test-data={out}"
    ]) as process:
        while not os.path.exists(distilled_path) and process.poll() == None:
            time.sleep(1)

        if ticket != '':
            with open(f"{out}/ticket.url", "w") as f:
                f.write(
                    f"https://github.com/brave/brave-browser/issues/{ticket}")

        time.sleep(1)
        process.terminate()
        process.kill()
    time.sleep(1)
    shutil.rmtree(temp_dir)
    return 0


def main(argv):
    parser = optparse.OptionParser(description=sys.modules[__name__].__doc__)
    parser.add_option('-b',
                      '--browser',
                      action='store',
                      type="string",
                      help='Browser executable')

    parser.add_option('-o',
                      '--out-dir',
                      action='store',
                      type="string",
                      help='Setup out dir')
    parser.add_option('-u',
                      '--urls-file',
                      action='store',
                      type="string",
                      help='Url')
    options, _ = parser.parse_args(argv)

    with open(options.urls_file, "r") as f:
        for url in f:
            collect_data(options.browser, options.out_dir, url.strip())

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
