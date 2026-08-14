// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program, Option } from 'commander'
import {
  createBuildConfigArgument,
  collect,
  parseBoolean,
} from '../lib/commandsUtils.ts'
import { build } from '../lib/build.ts'

program
  .option('-C <build_dir>', 'build directory, relative to out/ or absolute')
  .option(
    '--android_aab_to_apk',
    'applies an aab to apk conversion to the output aab',
  )
  .option(
    '--android_override_version_name <android_override_version_name>',
    'Android version number',
  )
  .option('--build_omaha', 'build omaha stub/standalone installer')
  .option(
    '--build_sparkle',
    'Build the Sparkle macOS update framework from source',
  )
  .addOption(
    new Option('--channel <target_channel>', 'target channel to build').choices(
      ['beta', 'dev', 'nightly', 'release'],
    ),
  )
  .option('--force_gn_gen', 'always run gn gen')
  .option(
    '--gn <arg>',
    'Additional gn args, in the form <key>:<value>',
    collect,
    [],
  )
  .option('--ignore_compile_failure', 'Keep compiling regardless of error')
  .option('--use_clang_coverage', 'enable coverage for brave source code')
  .option('--is_asan', 'is asan enabled')
  .option('--is_ubsan', 'is ubsan enabled')
  .option(
    '--last_chrome_installer <last_chrome_installer>',
    'folder contains previous version uncompressed chrome.7z pack file. This folder should be in out dir.',
  )
  .option(
    '--mac_installer_signing_identifier <id>',
    'The identifier to use for signing installers',
  )
  .option('--mac_signing_identifier <id>', 'The identifier to use for signing')
  .option(
    '--mac_signing_keychain <keychain>',
    'The identifier to use for signing',
    'login',
  )
  .option(
    '--ninja <opt>',
    'Additional Ninja command-line options, in the form <key>:<value>',
    collect,
    [],
  )
  .option('--notarize', 'notarize targets that support it with Apple')
  .option('--offline', 'use offline mode for RBE')
  .option(
    '--prepare_only',
    'Do not build targets, but prepare everything (build redirect_cc, update branding, etc.)',
  )
  .option('--skip_signing', 'skip signing binaries')
  .option('--tag_ap <ap>', 'ap for stub/standalone installer')
  .option(
    '--tag_installdataindex <index>',
    'installdataindex for stub/standalone installer',
  )
  .option(
    '--target <target>',
    'Comma-separated list of targets to build, instead of the default browser target',
  )
  // TODO(https://github.com/brave/brave-browser/issues/51200)
  // Cleanup target_android_base once cr144 reaches stable channel
  .option(
    '--target_android_base <target_android_base>',
    'Deprecated. Target Android SDK level for apk or aab (classic, modern, mono)',
    'classic',
  )
  .option(
    '--target_android_output_format <target_android_output_format>',
    'target Android output format (apk, aab)',
  )
  .option('--target_arch <target_arch>', 'target architecture')
  .option(
    '--target_environment <target_environment>',
    'target environment (device, catalyst, simulator)',
  )
  .option('--target_os <target_os>', 'target OS')
  .option('--universal', 'build a universal binary distribution')
  .option(
    '--use_remoteexec [arg]',
    'whether to use RBE for building',
    parseBoolean,
  )
  .option(
    '--xcode_gen <target>',
    'Generate an Xcode workspace ("ios" or a list of semi-colon separated label patterns, run `gn help label_pattern` for more info.',
  )
  .option(
    '--pkcs11-provider <provider_config_file>',
    'PKCS11 provider configuration file path',
  )
  .option('--pkcs11-alias <alias>', 'PKCS11 key alias')
  .addArgument(createBuildConfigArgument())
  .action(build)
  .parseAsync()
