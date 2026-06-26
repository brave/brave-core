// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command, Option } from 'commander'

export const command = new Command('transpile-web-ui')
  .requiredOption('--webpack_context_dir <path>', 'Webpack context directory')
  .requiredOption('--root_src_dir <path>', 'Chromium src root directory')
  .requiredOption('--root_gen_dir <path>', 'Root gen dir')
  .requiredOption('--output_dir <path>', 'Output directory')
  .requiredOption('--depfile_path <path>', 'Depfile path')
  .requiredOption('--import_paths_file <path>', 'Import paths file')
  .requiredOption('--grd_path <path>', 'GRD path')
  .requiredOption('--resource_name <name>', 'Resource name')
  .addOption(
    new Option('--mode <mode>', 'Webpack bundle mode')
      .makeOptionMandatory()
      .choices(['development', 'production']),
  )
  .requiredOption(
    '--entry <entry>',
    'Entry points',
    concatArray,
    [] as string[],
  )
  .option('--no_externals', 'Disable webpack externals')
  .option('--public_asset_path <path>', 'Public asset path')
  .option(
    '--webpack_alias <alias>',
    'Webpack alias',
    concatArray,
    [] as string[],
  )
  .option('--output_module', 'Output as ES module')
  .option('--resource_path_prefix <prefix>', 'Resource path prefix')
  .option(
    '--extra_modules <module>',
    'Extra module paths',
    concatArray,
    [] as string[],
  )
  .option('--sync_wasm', 'Use synchronous WASM loading')
  .option('--no_iife', 'Do not wrap output in IIFE')

export type TranspileWebUiCliOptions =
  typeof command extends Command<any[], infer Opts, any> ? Opts : never

function concatArray(value: string, previous: string[]): string[] {
  return previous.concat([value])
}
