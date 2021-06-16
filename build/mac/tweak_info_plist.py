#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

#
# Xcode supports build variable substitutions and CPP; sadly, that doesn't work
# because:
#
# 1. Xcode wants to do the Info.plist work before it runs any build phases,
#    this means if we were to generate a .h file for INFOPLIST_PREFIX_HEADER
#    we'd have to put it in another target so it runs in time.
# 2. Xcode also doesn't check to see if the header being used as a prefix for
#    the Info.plist has changed.  So even if we updated it, it's only looking
#    at the modtime of the info.plist to see if that's changed.
#
# So, we work around all of this by making a script build phase that will run
# during the app build, and simply update the info.plist in place.  This way
# by the time the app target is done, the info.plist is correct.
#

import optparse
import os
import plistlib
import re
import subprocess
import sys
import tempfile

def _ConvertPlist(source_plist, output_plist, fmt):
  """Convert |source_plist| to |fmt| and save as |output_plist|."""
  return subprocess.call(
      ['plutil', '-convert', fmt, '-o', output_plist, source_plist])


def _RemoveKeys(plist, *keys):
  """Removes a varargs of keys from the plist."""
  for key in keys:
    try:
      del plist[key]
    except KeyError:
      pass


def _OverrideVersionKey(plist, brave_version):
  """ `minor.build` version string is used for update.
  When we begin to use the Major version component, Brave version string will
  be `1.0.0` for example and `Minor.Build` (`0.0`) would be used for update
  check. Without modifying these numbers, update will fail as `0.0` is lower
  than `70.121` for example.

  To ensure minor version is higher than existing minor versions, we can
  multiply the major version by 100 and set it to `CFBundleVersion`."""
  version_values = brave_version.split('.')
  if int(version_values[0]) >= 1:
    adjusted_minor = int(version_values[1]) + (100 * int(version_values[0]))
    plist['CFBundleVersion'] = str(adjusted_minor) + '.' + version_values[2]

  # The "short" version is only used for display purposes. Chromium uses four
  # numbers w.x.y.z while Brave uses only three. So override the entry with our
  # value. This is in particular displayed in Brave's (/Sparkle's) update
  # notification popup: "Version X is available. You have version Y."
  plist['CFBundleShortVersionString'] = brave_version


def Main(argv):
  parser = optparse.OptionParser('%prog [options]')
  parser.add_option('--plist', dest='plist_path', action='store',
      type='string', default=None, help='The path of the plist to tweak.')
  parser.add_option('--output', dest='plist_output', action='store',
      type='string', default=None, help='If specified, the path to output ' + \
      'the tweaked plist, rather than overwriting the input.')
  parser.add_option('--brave_channel', dest='brave_channel', action='store',
      type='string', default=None, help='Channel (beta, dev, nightly)')
  parser.add_option('--brave_product_dir_name', dest='brave_product_dir_name',
      action='store', type='string', default=None,
      help='Product directory name')
  parser.add_option('--brave_eddsa_key', dest='brave_eddsa_key', action='store',
      type='string', default=None, help='Public EdDSA key for update')
  parser.add_option('--brave_version', dest='brave_version', action='store',
      type='string', default=None, help='brave version string')
  parser.add_option('--format', choices=('binary1', 'xml1', 'json'),
      default='xml1', help='Format to use when writing property list '
          '(default: %(default)s)')
  parser.add_option('--skip_signing', dest='skip_signing', action='store_true')
  (options, args) = parser.parse_args(argv)

  if len(args) > 0:
    print >>sys.stderr, parser.get_usage()
    return 1

  if not options.plist_path:
    print >>sys.stderr, 'No --plist specified.'
    return 1

  # Read the plist into its parsed format. Convert the file to 'xml1' as
  # plistlib only supports that format in Python 2.7.
  with tempfile.NamedTemporaryFile() as temp_info_plist:
    retcode = _ConvertPlist(options.plist_path, temp_info_plist.name, 'xml1')
    if retcode != 0:
      return retcode
    plist = plistlib.readPlist(temp_info_plist.name)

  output_path = options.plist_path
  if options.plist_output is not None:
    output_path = options.plist_output

  if options.skip_signing:
    plist['KSChannelID'] = options.brave_channel
  elif 'KSChannelID' in plist:
    # 'KSChannelID' is set at _modify_plists() of modification.py.
    del plist['KSChannelID']

  plist['CrProductDirName'] = options.brave_product_dir_name

  if options.brave_eddsa_key:
    plist['SUPublicEDKey'] = options.brave_eddsa_key

  _OverrideVersionKey(plist, options.brave_version)

  # Explicitly disable profiling
  plist['SUEnableSystemProfiling'] = False

  # Explicitly change notifications from banner to alert
  plist['NSUserNotificationAlertStyle'] = 'alert'

  # Now that all keys have been mutated, rewrite the file.
  with tempfile.NamedTemporaryFile() as temp_info_plist:
    plistlib.writePlist(plist, temp_info_plist.name)

    # Convert Info.plist to the format requested by the --format flag. Any
    # format would work on Mac but iOS requires specific format.
    return _ConvertPlist(temp_info_plist.name, output_path, options.format)


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
