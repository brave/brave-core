// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command, Option } from 'commander'

export const command = new Command('transpile-web-ui')
  .requiredOption(
    '--webpack_context_dir <path>',
    'The base directory for resolving entry points and loaders. Usually the path of "//brave"',
  )
  .requiredOption('--root_src_dir <path>', 'Chromium src root directory')
  .requiredOption(
    '--root_gen_dir <path>',
    'Root gen directory to locate other output dependencies',
  )
  .requiredOption(
    '--output_dir <path>',
    'Directory to create build output in (all contents will be deleted first)',
  )
  .requiredOption(
    '--depfile_path <path>',
    'Path to generate depfile at for use by GN to know which source files were used',
  )
  .requiredOption(
    '--import_paths_file <path>',
    'JSON file containing the import paths which describe the folders and files that are going to be used by webpack. The command will verify no other locations are used.',
  )
  .requiredOption(
    '--grd_path <path>',
    'Path to create a GRD or GRDP file with all the output contents',
  )
  .requiredOption(
    '--resource_name <name>',
    'Forms part of generated grit IDs, C++ file names and variables',
  )
  .addOption(
    new Option('--mode <mode>', 'Webpack bundle mode')
      .makeOptionMandatory()
      .choices(['development', 'production']),
  )
  .requiredOption(
    '--entry <entry>',
    'Entry points each in the form name=path',
    concatArray,
    [] as string[],
  )
  .option(
    '--no_externals',
    "Don't allow replacing imports with shared externals, e.g. React from chrome://resources",
  )
  .option(
    '--public_asset_path <path>',
    'Support a different URL path to access generated resources at',
  )
  .option(
    '--webpack_alias <alias>',
    'Webpack alias list to pass platform hint to libraries, possibly filtering imports',
    concatArray,
    [] as string[],
  )
  .option('--output_module', 'Output as ES module and prevent IIFE')
  .option(
    '--resource_path_prefix <prefix>',
    'Prefix to "serve" the resources from via GRD',
  )
  .option(
    '--extra_modules <module>',
    'Tell webpack what extra directories should be searched when resolving modules',
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
