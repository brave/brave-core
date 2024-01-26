#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Actions to run before every build in the brave-ios Xcode project
"""

import argparse
import os
import subprocess
import platform
import sys
import re

this_dir = os.path.dirname(os.path.abspath(__file__))
brave_root_dir = os.path.join(
    os.path.join(os.path.join(this_dir, os.pardir), os.pardir), os.pardir)
src_dir = os.path.join(brave_root_dir, os.pardir)


def main():
    description = 'Runs actions before each build'
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('--configuration',
                        nargs='?',
                        default='Debug',
                        help='Specify which configuration to build.')
    parser.add_argument('--platform_name',
                        nargs='?',
                        help='Specify which platform to build.')

    (options, _) = parser.parse_known_args()

    # Passed in configuration is going to be based on Xcode configurations which
    # is based on channels, so use Release for all non-Debug configs.
    config = 'Debug' if options.configuration == 'Debug' else 'Release'
    output_dir = BuildOutputDirectory(config, options.platform_name)
    target_arch = 'arm64' if platform.processor(
    ) == 'arm' or options.platform_name == 'iphoneos' else 'x64'
    target_environment = 'simulator' if (options.platform_name
                                         == 'iphonesimulator') else None

    UpdateSymlink(config, target_arch, target_environment)
    BuildCore(config, target_arch, target_environment)
    GenerateXCFrameworks(config, target_arch, target_environment)
    CleanupChromiumAssets(output_dir)
    FixMaterialComponentsVersionString(output_dir)
    GenerateXcodeConfig(output_dir)
    CallNpm(['npm', 'run', 'ios_pack_js'])


def BuildOutputDirectory(config, platform_name):
    directory_name = 'ios_%s' % config
    if platform.processor() == 'arm' or platform_name == 'iphoneos':
        directory_name += '_arm64'
    if platform_name == 'iphonesimulator':
        directory_name += '_simulator'
    return os.path.join(os.path.join(src_dir, 'out'), directory_name)


def UpdateSymlink(config, target_arch, target_environment):
    """Updates the 'current_link' symlink"""
    cmd_args = [
        'npm', 'run', 'update_symlink', '--', config, '--symlink_dir',
        os.path.join(src_dir, 'out/current_link'), '--target_os', 'ios',
        '--target_arch', target_arch
    ]
    if target_environment != None:
        cmd_args += ['--target_environment', target_environment]
    CallNpm(cmd_args)


def BuildCore(config, target_arch, target_environment):
    """Generates and builds the BraveCore.framework"""
    cmd_args = [
        'npm',
        'run',
        'build',
        '--',
        config,
        '--target_os',
        'ios',
        '--target_arch',
        target_arch,
    ]
    if target_environment != None:
        cmd_args += ['--target_environment', target_environment]
    CallNpm(cmd_args)


def _FrameworksForXCFramework(xcframework_dir):
    """Returns a list of framework directories inside of a xcframework"""
    framework_dirs = []
    for root, dirs, _ in os.walk(xcframework_dir):
        for folder in dirs:
            if folder.endswith('.framework'):
                framework_dirs += [
                    os.path.join(xcframework_dir, os.path.join(root, folder))
                ]
    return framework_dirs


def CleanupChromiumAssets(output_dir):
    """Delete Chromium Assets from BraveCore.xcframework since they aren't
    used."""
    # TODO(@brave/ios): Get this removed in the brave-core builds if possible
    xcframework_dir = os.path.join(output_dir, "BraveCore.xcframework")
    for framework_dir in _FrameworksForXCFramework(xcframework_dir):
        os.remove(os.path.join(framework_dir, "Assets.car"))


def FixMaterialComponentsVersionString(output_dir):
    """Adds a CFBundleShortVersionString to the outputted MaterialComponents
    xcframework so that it's valid to upload"""
    # TODO(@brave/ios): Fix this with a patch or chromium_src override somehow
    xcframework_dir = os.path.join(output_dir, "MaterialComponents.xcframework")
    for framework_dir in _FrameworksForXCFramework(xcframework_dir):
        cmd_args = [
            '/usr/libexec/PlistBuddy', '-c',
            'Add :CFBundleShortVersionString string 1.0',
            os.path.join(framework_dir, 'Info.plist')
        ]
        subprocess.call(cmd_args, cwd=brave_root_dir)


def GenerateXCFrameworks(config, target_arch, target_environment):
    """Generates xcframeworks for BraveCore & MaterialComponents"""
    cmd_args = [
        'npm', 'run', 'ios_create_xcframeworks', '--', config, '--target_arch',
        target_arch
    ]
    if target_environment != None:
        cmd_args += ['--target_environment', target_environment]
    CallNpm(cmd_args)


def GenerateXcodeConfig(output_dir):
    """Creates an xcconfig file filled with some gn args"""
    # Since not all gn args can be represented in Xcode config files we need to
    # specially copy out specific ones we want.
    copy_args = [
        'brave_version_major',
        'brave_version_minor',
        'brave_ios_marketing_version_patch',
        'brave_version_build',
        'brave_services_key',
        'brave_stats_api_key',
    ]
    xcconfig = []
    pattern = re.compile("^([^ =]+) =\n*(.+)", re.MULTILINE)
    patch_number = "0"
    strip_absolute_paths_from_debug_symbols = False
    with open(os.path.join(output_dir, 'args.gn'), 'r') as f:
        args = pattern.findall(f.read())
        for arg in args:
            if len(arg) < 2:
                continue
            (key, value) = (arg[0].strip(), arg[1].strip('" '))
            if key == 'brave_ios_marketing_version_patch':
                patch_number = value
            if (key == 'strip_absolute_paths_from_debug_symbols'
                    and value == 'true'):
                strip_absolute_paths_from_debug_symbols = True
            if key in copy_args:
                xcconfig.append('%s = %s' % (key, value))
    # Some special logic to avoid .0 patch versions in marketing versions
    marketing_version = "$(brave_version_major).$(brave_version_minor)"
    if patch_number != "0":
        marketing_version += ".$(brave_ios_marketing_version_patch)"
    with open(os.path.join(output_dir, 'args.xcconfig'), 'w') as f:
        f.write('\n'.join(xcconfig))
        f.write('\nbrave_ios_marketing_version = %s' % marketing_version)
        if strip_absolute_paths_from_debug_symbols:
            debug_prefix_map = [
                '-debug-prefix-map',
                '%s=../../brave/ios/brave-ios' % os.path.normpath(
                    os.path.join(src_dir, 'brave', 'ios', 'brave-ios'))
            ]
            f.write('\nbrave_ios_debug_prefix_map_flag = %s' %
                    ' '.join(debug_prefix_map))
    # Force Xcode to reload the SPM Package
    subprocess.call([
        'touch',
        os.path.join(brave_root_dir, 'ios', 'brave-ios', 'Package.swift')
    ])


def CallNpm(cmd):
    retcode = subprocess.call(cmd, cwd=brave_root_dir, stderr=subprocess.STDOUT)
    if retcode:
        raise subprocess.CalledProcessError(retcode, cmd)


if __name__ == '__main__':
    sys.exit(main())
