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

  out = out_dir + "/" + urlparse(url.strip()).netloc
  out = os.path.abspath(out)

  if os.path.exists(out + "/distilled.html") :
    print("Skip " + urlparse(url).netloc)
    return 0

  temp_dir = tempfile.mkdtemp()
  time.sleep(1)

  print("Processing " + url + " to " + out)
  process = subprocess.Popen([browser, url, "--user-data-dir=" + temp_dir, "--speedreader-collect-test-data=" + out])
  
  time_counter = 0
  while not os.path.exists(out + "/distilled.html") and process.poll() == None:
    time.sleep(1)

  if ticket != '':
    with open(out + "/ticket.url", "w") as f :
      f.write("https://github.com/brave/brave-browser/issues/" + ticket)

  time.sleep(1)
  process.terminate()
  process.kill()
  time.sleep(1)
  shutil.rmtree(temp_dir)
  return 0


def main(argv):
    parser = optparse.OptionParser(description=sys.modules[__name__].__doc__)
    parser.add_option(
        '-b',
        '--browser',
        action='store', type="string",
        help='Browser executable')

    parser.add_option(
        '-o',
        '--out-dir',
        action='store', type="string",
        help='Setup out dir')
    parser.add_option(
        '-u',
        '--urls-file',
        action='store', type="string",
        help='Url')      
    options, args = parser.parse_args(argv)

    with open(options.urls_file, "r") as f:
      for url in f:
        collect_data(options.browser, options.out_dir, url.strip())

    return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
