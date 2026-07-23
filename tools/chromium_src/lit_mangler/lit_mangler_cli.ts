// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { program } from '@commander-js/extra-typings'
import { load, write } from './lit_mangler.ts'
import childProcess from 'child_process'
import path from 'path'
import fs from 'fs'
import { registerHooks } from 'module'
import { pathToFileURL } from 'url'
import ts from 'typescript'

const baseDir = path.join(import.meta.dirname, '../../../')
const tsConfigPath = path.join(import.meta.dirname, 'tsconfig.json')

/**
 * Unfortunately tsc doesn't support passing in a path map for a single file
 * check, so we have to generate a new tsconfig based on tsconfig-mangle.json
 * and the file we want to check.
 * @param file The file to generate the tsconfig for
 * @returns The path to the generated tsconfig
 */
const getTsConfigForFiles = (genDir: string, files: string[]) => {
  const { config: tsConfig, error } = ts.readConfigFile(
    tsConfigPath,
    ts.sys.readFile,
  )
  if (error) {
    console.error('Error reading tsconfig:', error)
    process.exit(1)
  }

  // Extend the tsconfig to include the build tsconfig.json.
  tsConfig.extends = path.join(baseDir, 'build', 'tsconfig.json')

  // Override the include path to only include the lit_mangler and the file
  // we want to check
  tsConfig.include = [
    path.join(import.meta.dirname, 'lit_mangler.ts'),
    ...files,
  ]

  // As the file is generated in the temp directory we need to set the baseUrl
  // so everything resolvese correctly.
  tsConfig.compilerOptions.baseUrl = baseDir
  // Pass typeRoots path explicitly to work correctly with symlinks in
  // node_modules (pnpm isolated mode).
  tsConfig.compilerOptions.typeRoots = [
    path.join(baseDir, 'node_modules/@types'),
  ]

  // Write the tsconfig to the gen directory.
  const tsConfigName = `lit-mangler-check-tsconfig_${files.map((file) => path.basename(file)).join('_')}.json`
  const tempTsConfigPath = path.join(genDir, tsConfigName)
  fs.writeFileSync(tempTsConfigPath, JSON.stringify(tsConfig, null, 2))
  return path.normalize(tempTsConfigPath)
}

const runTypecheck = (genDir: string, files: string[]) => {
  const result = childProcess.spawnSync(process.execPath, [
    path.join(baseDir, 'node_modules', 'typescript', 'bin', 'tsc'),
    '-p',
    getTsConfigForFiles(genDir, files),
  ])
  // Note: tsc in Windows doesn't return a status code on success (i.e. status === null).
  if (result.status ?? 0 !== 0) {
    console.error(
      'Typechecking failed:\n',
      result.stdout?.toString(),
      result.stderr?.toString(),
    )
    process.exit(1)
  }
}

program
  .command('mangle')
  .requiredOption(
    '-m, --mangler <file>',
    'The file with containing the mangler instructions',
  )
  .requiredOption('-i, --input <file>', 'The file to mangle')
  .requiredOption('-o, --output <file>', 'Where to output the mangled file')
  .requiredOption('-g, --gen-dir <folder>', 'The folder for generated files')
  .option('-t, --typecheck', 'Run typechecking before mangling')
  .action(async (options) => {
    load(options.input)

    // Typecheck the mangler file.
    if (options.typecheck) {
      runTypecheck(options.genDir, [options.mangler])
    }

    // Register import hook so `lit_mangler` can be imported by the mangler.
    const litManglerUrl = pathToFileURL(
      path.join(import.meta.dirname, 'lit_mangler.ts'),
    ).href
    const hooks = registerHooks({
      resolve(specifier, context, nextResolve) {
        if (specifier === 'lit_mangler') {
          return { shortCircuit: true, url: litManglerUrl }
        }
        return nextResolve(specifier, context)
      },
    })

    // Run the mangler.
    try {
      await import(pathToFileURL(options.mangler).href)
    } finally {
      hooks.deregister()
    }

    // Write the mangled file.
    write(options.output)
  })

program.parseAsync()
