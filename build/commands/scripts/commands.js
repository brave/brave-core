// Copyright (c) 2017 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Check environment before doing anything.
import '../lib/checkEnvironment.js'

import { program, Option } from 'commander'
import path from 'node:path'
import fs from 'fs-extra'
import config from '../lib/config.ts'
import util from '../lib/util.js'
import { buildFuzzer, runFuzzer } from '../lib/fuzzer.js'
import versions from '../lib/versions.js'
import start from '../lib/start.js'
import applyPatches from '../lib/applyPatches.js'
import updatePatches from './updatePatches.js'
import pullL10n from '../lib/pullL10n.js'
import pushL10n from '../lib/pushL10n.js'
import chromiumRebaseL10n from '../lib/chromiumRebaseL10n.js'
import gnCheck from '../lib/gnCheck.js'
import genGradle from '../lib/genGradle.js'
import perfTests from '../lib/perfTests.ts'
import registerListAffectedTestsCommand from './listAffectedTests.js'
import registerGenerateCoverageReportCommand from './generateCoverageReport.js'
import { parseInteger, parseBoolean } from '../lib/commandsUtils.ts'
import * as buildOptions from '../lib/buildOptions.ts'

const parsedArgs = program.parseOptions(process.argv)

// @ts-ignore
program.version(process.env.npm_package_version)

program.command('versions').action(versions)

program
  .command('gn_check')
  .option('-C <build_dir>', 'build config (out/Debug, out/Release')
  .option('--target_os <target_os>', 'target OS')
  .option('--target_arch <target_arch>', 'target architecture')
  // TODO(https://github.com/brave/brave-browser/issues/51200)
  // Cleanup target_android_base once cr144 reaches stable channel
  .option(
    '--target_android_base <target_android_base>',
    'Deprecated. Target Android OS apk (classic, modern, mono)',
    'classic',
  )
  .option(
    '--target_environment <target_environment>',
    'target environment (device, catalyst, simulator)',
  )
  .option('--checkdeps_only', 'only run checkdeps')
  .apply(buildOptions.supportBuildConfigArg)
  .action(gnCheck)

program
  .command('apply_patches')
  .option(
    '--print-patch-failures-in-json',
    'Emits a JSON structure with a list of patch files that failed to apply',
  )
  .apply(buildOptions.supportBuildConfigArg)
  .action(applyPatches)

program
  .command('update_symlink')
  .option(
    '--symlink_dir <symlink_dir>',
    'symlink that points to the actual build directory',
  )
  .addOption(
    new Option('--target_os <target_os_type>', 'target OS type').choices([
      'host_os',
      'ios',
      'android',
    ]),
  )
  .addOption(
    new Option('--target_arch <target_arch>', 'target architecture').choices([
      'host_cpu',
      'x64',
      'arm64',
      'x86',
    ]),
  )
  .addOption(
    new Option(
      '--target_environment <target_environment>',
      'target environment',
    ).choices(['device', 'catalyst', 'simulator']),
  )
  .apply(buildOptions.supportBuildConfigArg)
  .action(async (buildConfig, options) => {
    config.buildConfig = buildConfig || config.defaultBuildConfig
    if (options.target_os === 'host_os') {
      delete options.target_os
    }

    if (options.target_arch === 'host_cpu') {
      delete options.target_arch
    }

    config.update(options)
    // ignore use_no_gn_gen when updating the symlink
    delete config.use_no_gn_gen
    const currentLink = options.symlink_dir
    if (!currentLink) {
      console.error('Symlink directory is required')
      process.exit(1)
    }
    if (
      !path.isAbsolute(currentLink)
      && !path.relative(currentLink, config.srcDir).startsWith('..')
    ) {
      console.error('Symlink must be an absolute path in src')
      process.exit(1)
    }

    fs.removeSync(currentLink)
    fs.symlinkSync(config.outputDir, currentLink, 'junction')
    await util.generateNinjaFiles()
  })

program
  .command('start')
  .allowUnknownOption(true)
  .allowExcessArguments(true)
  .option('-C <build_dir>', 'build config (out/Debug, out/Release')
  .option('--v [log_level]', 'set log level to [log_level]', parseInteger, 0)
  .option('--vmodule [modules]', 'verbose log from specific modules')
  .option(
    '--user_data_dir_name [base_name]',
    'set user data directory base name to [base_name]',
  )
  .option('--no_sandbox', 'disable the sandbox')
  .option('--disable_brave_extension', 'disable loading the Brave extension')
  .option(
    '--disable_brave_rewards_extension',
    'disable loading the Brave Rewards extension',
  )
  .option('--disable_pdfjs_extension', 'disable loading the PDFJS extension')
  .addOption(
    new Option(
      '--ui_mode <ui_mode>',
      'which built-in ui appearance mode to use',
    ).choices(['dark', 'light']),
  )
  .option(
    '--show_component_extensions',
    'show component extensions in chrome://extensions',
  )
  .option('--enable_brave_update', 'enable brave update')
  .addOption(
    new Option('--channel <target_channel>', 'target channel to start').choices(
      ['beta', 'dev', 'nightly', 'release'],
    ),
  )
  .option('--official_build <official_build>', 'force official build settings')
  // See https://github.com/brave/brave-browser/wiki/Rewards#flags for more information
  .option('--rewards [options]', 'options for rewards')
  .option('--brave_ads_testing', 'ads testing')
  .option('--brave_ads_production', 'ads production')
  .option('--brave_ads_staging', 'ads staging')
  .option('--brave_ads_debug', 'ads debug')
  .option('--single_process', 'use a single process')
  .option('--use_real_keychain', "don't add --use-mock-keychain in macOS")
  .option(
    '--output_path [pathname]',
    'use the Brave binary located at [pathname]',
  )
  .apply(buildOptions.supportBuildConfigArg)
  .action(start.bind(null, parsedArgs.unknown))

program
  .command('pull_l10n')
  .option('--channel <channel>', 'Release|Beta|Nightly, Release by default')
  .option(
    '--grd_path <grd_path>',
    `Relative path to match end of full GRD path, e.g: 'generated_resources.grd'.`,
  )
  .option('--lang <language>', 'Only download content for this language')
  .option(
    '--debug',
    `Dumps downloaded content for one language into CrowdinCurrent.txt file in the temp directory.`,
  )
  .action(pullL10n)

program
  .command('push_l10n')
  .option('--channel <channel>', 'Release|Beta|Nightly, Release by default')
  .option(
    '--grd_path <grd_path>',
    `Relative path to match end of full GRD path, e.g: 'generated_resources.grd'.`,
  )
  .option(
    '--with_translations',
    'Push local translations. WARNING: this will overwrite translations in Crowdin.',
  )
  .option(
    '--with_missing_translations',
    'Push local translations for strings that do not have translations in Crowdin.',
  )
  .action(pushL10n)

program.command('chromium_rebase_l10n').action(chromiumRebaseL10n)

program
  .command('update_patches')
  .arguments('[filePaths...]')
  .description(
    'Updates all patches in the brave-core repo. If a filePath is provider, only that specific file will be updated.',
  )
  .option(
    '--no-plaster-check',
    'Allows regenerating patches, even for plaster managed sources.',
  )
  .action(updatePatches)

program.command('mass_rename').action(util.massRename)

program
  .command('build_fuzzer <fuzzer_test_target>')
  .option(
    '--use_remoteexec [arg]',
    'whether to use RBE for building',
    parseBoolean,
  )
  .option('--offline', 'use offline mode for RBE')
  .action(buildFuzzer)

program
  .command('run_fuzzer <suite>')
  .allowUnknownOption(true)
  .allowExcessArguments(true)
  .action(runFuzzer.bind(null, parsedArgs.unknown))

program
  .command('run_perf_tests [perf_config] [targets]')
  .option('--target_os <target_os>', 'target OS')
  .option('--target_arch <target_arch>', 'target architecture')
  .allowUnknownOption(true)
  .allowExcessArguments(true)
  .description('Call pnpm run perf_tests --more-help for detailed help')
  .action(perfTests.runPerfTests.bind(null, parsedArgs.unknown))

program
  .command('gen_gradle')
  .allowUnknownOption(true)
  .allowExcessArguments(true)
  .option('-C <build_dir>', 'build config (out/Debug, out/Release)')
  .option('--target_arch <target_arch>', 'target architecture')
  .apply(buildOptions.supportBuildConfigArg)
  .action(genGradle.bind(null, parsedArgs.unknown))

program.command('docs').action(util.launchDocs)

registerListAffectedTestsCommand(program)
registerGenerateCoverageReportCommand(program)

program.parse(process.argv)
