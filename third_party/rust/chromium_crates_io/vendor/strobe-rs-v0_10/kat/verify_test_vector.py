# This file verifies a JSON-encoded test vector against the STROBE reference implementation.

# This file is modified from David Wong's test file:
# https://gist.github.com/mimoo/64e5054f4927a0df0033c8f5f29f4029
# Thanks, David!

from __future__ import print_function
import binascii
import fileinput
import getopt
import json
import os
import sys

setup_instructions = \
"""How to run:
  1. Download a copy of STROBE  into the root directory of this crate and name
     the folder `strobe-reference` by running
     `git clone git://git.code.sf.net/p/strobe/code strobe-reference`
  2. `cd` back into this folder
  3. Run `python2 {} TEST_VECTOR_JSON_FILE`
""".format(sys.argv[0])

# Add Python Strobe module to path
sys.path.insert(0, "../strobe-reference/python")
try:
    from Strobe.Strobe import *
except:
    print("Error: You forgot to import the STROBE Python reference implemention.")
    print(setup_instructions)
    sys.exit(1)

# Op flags
I,A,C,T,M,K = 1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5

def create_flag(name):
    if name == "AD":
        return A
    elif name == "KEY":
        return A|C
    elif name == "PRF":
        return I|A|C
    elif name == "send_CLR":
        return A|T
    elif name == "recv_CLR":
        return I|A|T
    elif name == "send_ENC":
        return A|C|T
    elif name == "recv_ENC":
        return I|A|C|T
    elif name == "send_MAC":
        return C|T
    elif name == "recv_MAC":
        return I|C|T
    elif name == "RATCHET":
        return C
    else:
        print("Op not recognized: {}".format(name))
        sys.exit(1)

def test(filename):
    counter = 0
    test_vector = json.load(open(filename))
    ss = Strobe(bytearray(test_vector["proto_string"], "utf8"), security=test_vector["security"])
    for oo in test_vector["operations"]:
        if oo["name"] == "init":
            assert(oo["state_after"] == binascii.hexlify(ss.st))
        else:
            flag = create_flag(oo["name"])
            input_data = binascii.unhexlify(oo["input_data"])

            if oo["meta"]:
                flag |= M

            output = None
            try:
                # If the operation takes integral inputs, give it the length of the input
                if (flag & (I|T) != (I|T)) and (flag & (I|A) != A):
                    output = ss.operate(flags=flag, data=len(input_data), more=oo["stream"])
                else:
                    output = ss.operate(flags=flag, data=input_data, more=oo["stream"])
            except AuthenticationFailed:
                # We do not expect recv_MAC to succeed on random inputs
                pass

            assert(binascii.hexlify(ss.st) == oo["state_after"])
            if "output" in oo:
                assert(binascii.hexlify(output) == oo["output"])

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("\nUsage:\npython2 run_test.py TEST_VECTOR_JSON_FILE\n")
        sys.exit(1)

    filename = sys.argv[1]
    test(filename)
    print("Test on {} passed".format(os.path.basename(filename)))
