import fnmatch
import os
import subprocess

pfx_path = os.environ.get('CERT')
pfx_pass = os.environ.get('CERT_PASSWORD')

assert pfx_path, "CERT required for signing"
assert pfx_pass , "CERT_PASSWORD required for signing"

def get_sign_cmd(file):
  # https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
  return ('signtool ' +
       ' sign /t  http://timestamp.verisign.com/scripts/timstamp.dll' +
       ' /fd sha256' +
       ' /f "' + pfx_path +
       '" /p "' + pfx_pass+ '" ' +
       file)

def run_cmd(cmd):
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  for line in p.stdout:
    print line
  p.wait()
  assert(p.returncode == 0, "Error signing")

def sign_binaries(base_dir):
  matches = []
  for root, dirnames, filenames in os.walk(base_dir):
    for filename in filenames:
      if filename.endswith(('.exe', '.dll')):
        matches.append(os.path.join(root, filename))

  for binary in matches:
    sign_binary(binary)

def sign_binary(binary):
  cmd = get_sign_cmd(binary)
  run_cmd(cmd)
