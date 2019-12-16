# Copyright 2019 The Brave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import subprocess
import tempfile

if len(sys.argv) != 8:
  print("Incorrect number of parameters")
  print("Usage: " + sys.argv[0] + " <zipalign_path>, <apksigner_path>, <unsigned_apk_path>, <key_path>, <key_passwd>, <prvt_key_passwd>, <key_name>")
  sys.exit(1)

zipalign_path = sys.argv[1]
apksigner_path = sys.argv[2]
unsigned_apk_path = sys.argv[3]
key_path = sys.argv[4]
key_passwd = sys.argv[5]
prvt_key_passwd = sys.argv[6]
key_name = sys.argv[7]

with tempfile.NamedTemporaryFile() as staging_file:
  subprocess.check_output([
      zipalign_path, '-p', '-f', '4',
      unsigned_apk_path, staging_file.name])
  subprocess.check_output([
      apksigner_path, 'sign',
      '--in', staging_file.name,
      '--out', unsigned_apk_path,
      '--ks', key_path,
      '--ks-key-alias', key_name,
      '--ks-pass', 'pass:' + key_passwd,
      '--key-pass', 'pass:' + prvt_key_passwd,
  ])