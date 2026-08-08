// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { Command, Option, type OptionValues } from 'commander'
import {
  createBuildConfigArgument,
  collect,
  argParserBoolean,
} from '../lib/commandsUtils.ts'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import branding from '../lib/branding.js'
import * as buildUtils from '../lib/buildUtils.ts'

type OptionsOf<T> =
  T extends Command<unknown[], infer Options, OptionValues> ? Options : never

export function addBuildConfigOptions<
  Args extends unknown[],
  Opts extends OptionValues,
  GlobalOpts extends OptionValues,
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .addArgument(createBuildConfigArgument())
    .option('-C <build_dir>', 'build directory, relative to out/ or absolute')
}

export function addGnArgsOptions<
  Args extends unknown[],
  Opts extends OptionValues,
  GlobalOpts extends OptionValues,
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option(
      '--gn <arg>',
      'Additional gn args, in the form <key>:<value>',
      collect,
    )
    .addOption(
      new Option('--target_os <target_os>', 'target OS type').choices([
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
      new Option(
        '--target_cpu, --target_arch <target_cpu>',
        'target architecture',
      ).choices(['arm', 'arm64', 'ia32', 'x64', 'x86']),
    )
    .addOption(
      new Option(
        '--target_environment <target_environment>',
        'target environment',
      ).choices(['device', 'catalyst', 'simulator']),
    )
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
    .option(
      '--target_android_output_format <target_android_output_format>',
      'target Android output format (apk, aab)',
    )
    .addOption(
      new Option('--use_remoteexec [arg]', 'whether to use RBE for building')
        .choices(['true', 'false'])
        .argParser(argParserBoolean),
    )
    .option('--universal', 'build a universal binary distribution')
    .option(
      '--pkcs11_provider, --pkcs11-provider <provider_config_file>',
      'PKCS11 provider configuration file path',
    )
    .option('--pkcs11_alias, --pkcs11-alias <alias>', 'PKCS11 key alias')
}

export type BuildConfigOptions = OptionsOf<
  ReturnType<typeof addBuildConfigOptions<[], {}, {}>>
>

export type GnArgsOptions = OptionsOf<
  ReturnType<typeof addGnArgsOptions<[], {}, {}>>
>

export function addGnGenOptions<
  Args extends unknown[],
  Opts extends OptionValues,
  GlobalOpts extends OptionValues,
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option('--force_gn_gen', 'always run gn gen')
    .option(
      '--xcode_gen <target>',
      'Generate an Xcode workspace ("ios" or a list of semi-colon separated label patterns, run `gn help label_pattern` for more info.',
    )
}

export type GnGenOptions = OptionsOf<
  ReturnType<typeof addGnGenOptions<[], {}, {}>>
>

export function addBuildOptions<
  Args extends unknown[],
  Opts extends OptionValues,
  GlobalOpts extends OptionValues,
>(command: Command<Args, Opts, GlobalOpts>) {
  return command
    .option('--ignore_compile_failure', 'Keep compiling regardless of error')
    .option(
      '--ninja <opt>',
      'Additional Ninja command-line options, in the form <key>:<value>',
      collect,
    )
    .option('--offline', 'use offline mode for RBE')
    .option(
      '--prepare_only',
      'Do not build targets, but prepare everything (build redirect_cc, update branding, etc.)',
    )
    .option(
      '--target <target>',
      'Comma-separated list of targets to build, instead of the default browser target',
    )
}

export type BuildOptions = OptionsOf<
  ReturnType<typeof addBuildOptions<[], {}, {}>>
>

type GnOptionName = keyof GnArgsOptions
const gnOptionNameSet = {
  gn: true,
  target_os: true,
  target_arch: true,
  target_environment: true,
  channel: true,
  android_aab_to_apk: true,
  android_override_version_name: true,
  build_omaha: true,
  build_sparkle: true,
  use_clang_coverage: true,
  is_asan: true,
  is_ubsan: true,
  last_chrome_installer: true,
  mac_installer_signing_identifier: true,
  mac_signing_identifier: true,
  mac_signing_keychain: true,
  notarize: true,
  skip_signing: true,
  tag_ap: true,
  tag_installdataindex: true,
  target_android_output_format: true,
  use_remoteexec: true,
  universal: true,
  pkcs11Provider: true,
  pkcs11Alias: true,
} as const satisfies Record<GnOptionName, true>

export const gnOptionNames = Object.keys(gnOptionNameSet) as GnOptionName[]

export async function build(
  buildConfig: string,
  options: BuildConfigOptions & GnArgsOptions & GnGenOptions & BuildOptions,
) {
  config.buildConfig = buildConfig ?? config.defaultBuildConfig
  config.update(options)
  buildUtils.checkVersionsMatch()

  util.touchOverriddenFiles()
  branding.update()
  buildUtils.ensureVsFilesMount()
  await util.buildNativeRedirectCC()

  if (options.prepare_only) {
    return
  }

  if (config.xcode_gen_target) {
    util.generateXcodeWorkspace()
  } else {
    if (!config.use_no_gn_gen) {
      await util.generateNinjaFiles()
    }
    await util.buildTargets()
  }
}
