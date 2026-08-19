// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Argument, Command, Option } from 'commander'
import './commanderApply.ts'
import { collect, parseBoolean, parseInteger } from './commandsUtils.ts'

export function supportBuildConfigArg<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command.addArgument(
    new Argument('[build_config]', 'build configuration').argParser((value) =>
      value.startsWith('-') ? undefined : value,
    ),
  )
}

export function supportBuildDir<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command.option(
    '-C <build_dir>',
    'build directory, relative to src/out or absolute',
  )
}

export function supportTargetConfig<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .addOption(
      new Option('--target_os <target_os>', 'target OS type').choices([
        'host_os',
        'android',
        'ios',
        'linux',
        'mac',
        'macos',
        'win',
        'windows',
      ]),
    )
    .addOption(
      new Option('--target_arch <target_cpu>', 'target architecture').choices([
        'host_cpu',
        'arm',
        'arm64',
        'ia32',
        'x64',
        'x86',
      ]),
    )
    .addOption(
      new Option(
        '--target_environment <target_environment>',
        'target environment',
      ).choices(['device', 'catalyst', 'simulator']),
    )
}

export function supportGnArgs<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return (
    command
      .addOption(
        new Option(
          '--channel <target_channel>',
          'target channel to build',
        ).choices(['beta', 'dev', 'nightly', 'release']),
      )
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
      .option(
        '--mac_signing_identifier <id>',
        'The identifier to use for signing',
      )
      .option(
        '--mac_signing_keychain <keychain>',
        'The identifier to use for signing',
      )
      .option('--notarize', 'notarize targets that support it with Apple')
      .option('--skip_signing', 'skip signing binaries')
      .option('--tag_ap <ap>', 'ap for stub/standalone installer')
      .option(
        '--tag_installdataindex <index>',
        'installdataindex for stub/standalone installer',
      )
      // TODO(https://github.com/brave/brave-browser/issues/51200)
      // Cleanup target_android_base once cr144 reaches stable channel
      .option(
        '--target_android_base <target_android_base>',
        'Deprecated. Target Android SDK level for apk or aab (classic, modern, mono)',
      )
      .option(
        '--target_android_output_format <target_android_output_format>',
        'target Android output format (apk, aab)',
      )
      .addOption(
        new Option('--use_remoteexec [arg]', 'whether to use RBE for building')
          .choices(['true', 'false'])
          .argParser(parseBoolean),
      )
      .option('--universal', 'build a universal binary distribution')
      .option(
        '--pkcs11-provider <provider_config_file>',
        'PKCS11 provider configuration file path',
      )
      .option('--pkcs11-alias <alias>', 'PKCS11 key alias')
      .option('--use_libfuzzer', 'enable fuzzer binaries')
      .apply(supportExtraGnArgs)
  )
}

export function supportExtraGnArgs<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command.option(
    '--gn <arg>',
    'Additional gn args, in the form <key>:<value>',
    collect,
  )
}

export function supportGnGenOptions<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option('--force_gn_gen', 'always run gn gen')
    .option(
      '--xcode_gen <target>',
      'Generate an Xcode workspace ("ios" or a list of semi-colon separated label patterns, run `gn help label_pattern` for more info.',
    )
}

export function supportNinjaOptions<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option('--ignore_compile_failure', 'Keep compiling regardless of error')
    .option('--offline', 'use offline mode for RBE')
    .option(
      '--prepare_only',
      'Do not build targets, but prepare everything (build redirect_cc, update branding, etc.)',
    )
    .option(
      '--target <target>',
      'Comma-separated list of targets to build, instead of the default browser target',
    )
    .apply(supportExtraNinjaOptions)
}

export function supportExtraNinjaOptions<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command.option(
    '--ninja <opt>',
    'Additional Ninja command-line options, in the form <key>:<value>',
    collect,
  )
}

export function supportTestOptions<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option('--v [log_level]', 'set log level to [log_level]', parseInteger, 0)
    .option('--vmodule [modules]', 'verbose log from specific modules')
    .option('--filter <filter>', 'set test filter')
    .option(
      '--base [targetCommitRef]',
      'use this commit/branch/tag as reference for change detection',
    )
    .option(
      '--output_xml',
      'indicates if test results xml output file(s) should be generated. '
        + '<suite>.txt file will contain the list of xml files with results. '
        + 'All output files are generated in the src directory',
    )
    .option('--quiet', 'enable quiet mode')
    .option('--disable_brave_extension', 'disable loading the Brave extension')
    .option(
      '--single_process',
      'uses a single process to run tests to help with debugging',
    )
    .option(
      '--test_launcher_jobs <test_launcher_jobs>',
      'Number of jobs to launch',
      parseInteger,
      4,
    )
    .option('--run_disabled_tests', 'run disabled tests')
    .option(
      '--manual_android_test_device',
      'indicates that Android test device is run manually',
    )
    .option(
      '--android_test_emulator_name <emulator_name>',
      'set name of the Android emulator for tests',
      'android_33_google_apis_x64',
    )
    .option(
      '--ios_xcode_build_version <build_version>',
      'xcode build version for ios',
    )
    .option(
      '--ios_simulator_platform <simulator_platform>',
      'platform to use for ios simulator',
      'iPhone 17',
    )
    .option(
      '--ios_simulator_version <simulator_version>',
      'ios version for simulator',
      '26.2',
    ) // should match ios_deployment_target
}

type OptionsOf<
  F extends (command: Command<[], {}, {}>) => Command<any, any, any>,
> = ReturnType<F> extends Command<any, infer Options, any> ? Options : never

export type BuildDirOptions = OptionsOf<typeof supportBuildDir>
export type TargetConfigOptions = OptionsOf<typeof supportTargetConfig>
export type GnArgsOptions = OptionsOf<typeof supportGnArgs>
export type ExtraGnArgsOptions = OptionsOf<typeof supportExtraGnArgs>
export type GnGenOptions = OptionsOf<typeof supportGnGenOptions>
export type NinjaOptions = OptionsOf<typeof supportNinjaOptions>
export type ExtraNinjaOptions = OptionsOf<typeof supportExtraNinjaOptions>
export type TestOptions = OptionsOf<typeof supportTestOptions>
