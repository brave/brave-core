import optparse
import os
import subprocess
import sys

cert = os.environ.get('CERT')
signtool_args = (os.environ.get('SIGNTOOL_ARGS') or
                 'sign /t http://timestamp.digicert.com /sm '
                 '/fd sha256')

assert (cert or signtool_args), 'One or both of the CERT or SIGNTOOL_ARGS '
'must be set. CERT by default is the name in the //CurrentUser/My windows '
'certificate store. `SIGNTOOL_ARGS` can be used in combination `CERT` or '
'by it self.'


def get_sign_cmd(file):
    # https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
    # signtool should be in the path if it was set up correctly by gn through
    # src/build/vs_toolchain.py
    cmd = 'signtool {}'.format(signtool_args)
    if cert:
        cmd = cmd + ' /n "' + cert + '"'
    return (cmd + ' "' + file + '"')


def run_cmd(cmd):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    for line in p.stdout:
        print(line)
    p.wait()
    assert p.returncode == 0, "Error signing"


def sign_binaries(base_dir, endswidth=('.exe', '.dll')):
    matches = []
    for root, dirnames, filenames in os.walk(base_dir):
        for filename in filenames:
            if filename.endswith(endswidth):
                matches.append(os.path.join(root, filename))

    for binary in matches:
        sign_binary(binary)


def sign_binary(binary):
    cmd = get_sign_cmd(binary)
    run_cmd(cmd)


def _ParseOptions():
    parser = optparse.OptionParser()
    parser.add_option(
        '-b', '--build_dir',
        help='Build directory. The paths in input_file are relative to this.')

    options, _ = parser.parse_args()
    if not options.build_dir:
        parser.error('You must provide a build dir.')

    options.build_dir = os.path.normpath(options.build_dir)

    return options


def main(options):
    sign_binaries(options.build_dir, ('brave.exe', 'chrome.dll'))


if '__main__' == __name__:
    options = _ParseOptions()
    sys.exit(main(options))
