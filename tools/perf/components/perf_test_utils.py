# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
import logging
import os
import subprocess
import tempfile
import platform

from threading import Timer
from typing import List, Optional, Tuple
from urllib.request import urlopen

import components.path_util as path_util

with path_util.SysPath(path_util.GetBraveScriptDir(), 0):
  from lib.util import extract_zip


def ToChromiumPlatformName(target_os: str) -> str:
  if target_os == 'mac':
    return 'mac-arm64' if platform.processor() == 'arm' else 'mac-x64'
  if target_os == 'windows':
    return 'win64'
  if target_os == 'linux':
    return 'linux64'
  if target_os == 'android':
    return 'android-arm64'
  raise RuntimeError('Platform is not supported')


def ToBravePlatformName(target_os: str) -> str:
  if target_os == 'mac':
    return 'darwin-arm64' if platform.processor() == 'arm' else 'darwin-x64'
  if target_os == 'windows':
    return 'win32-x64'
  if target_os == 'linux':
    return 'linux-x64'
  if target_os == 'android':
    return 'android-arm64'
  raise RuntimeError('Platform is not supported')


def TerminateProcess(p):
  logging.error('terminating process by timeout %r', p.args)
  p.terminate()


def GetProcessOutput(args: List[str],
                     cwd: Optional[str] = None,
                     check=False,
                     timeout: Optional[int] = None) -> Tuple[bool, str]:
  if logging.root.isEnabledFor(logging.DEBUG):
    logging.debug('Run binary: %s, cwd = %s  output:', ' '.join(args), cwd)
    process = subprocess.Popen(args,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               env=os.environ,
                               cwd=cwd,
                               bufsize=0,
                               universal_newlines=True)
    timer = None
    if timeout:
      timer = Timer(timeout, lambda: TerminateProcess(process))
    output = ''
    try:
      if timer:
        timer.start()
      while True:
        assert process.stdout is not None
        line = process.stdout.readline()
        if line:
          output += line
          logging.debug(line.rstrip())
        if not line and process.poll() is not None:
          break
    finally:
      if timer:
        timer.cancel()
    rc = process.poll()
    if check and rc != 0:
      logging.debug('Binary failed. Exit code: %d', rc)
      if not rc:
        rc = -1
      raise subprocess.CalledProcessError(rc, args, output)
    return rc == 0, output

  try:
    output = subprocess.check_output(args,
                                     stderr=subprocess.STDOUT,
                                     cwd=cwd,
                                     env=os.environ,
                                     timeout=timeout,
                                     universal_newlines=True)
    return True, output
  except subprocess.CalledProcessError as e:
    logging.error(e.output)
    if check:
      raise
    return False, e.output


def DownloadFile(url: str, output: str):
  logging.debug('Downloading %s', url)
  f = urlopen(url)
  data = f.read()
  with open(output, 'wb') as output_file:
    output_file.write(data)


def DownloadArchiveAndUnpack(output_directory: str, url: str):
  _, f = tempfile.mkstemp(dir=output_directory)
  DownloadFile(url, f)
  extract_zip(f, output_directory)
