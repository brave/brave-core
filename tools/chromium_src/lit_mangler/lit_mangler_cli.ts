// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import commander from 'commander'
import { load, write } from './lit_mangler'
import childProcess from 'child_process'
import path from 'path'
import fs from 'fs'

const baseDir = path.join(__dirname, '../../../')
const tsConfigPath = path.join(baseDir, 'tsconfig-mangle.json')

/**
 * Unfortunately tsc doesn't support passing in a path map for a single file
 * check, so we have to generate a new tsconfig based on tsconfig-mangle.json
 * and the file we want to check.
 * @param file The file to generate the tsconfig for
 * @returns The path to the generated tsconfig
 */
const getTsConfigForFiles = (genDir: string, files: string[]) => {
  const tsConfig = JSON.parse(fs.readFileSync(tsConfigPath, 'utf8'))

  // Override the include path to only include the lit_mangler and the file
  // we want to check
  tsConfig.include = [path.join(__dirname, 'lit_mangler.ts'), ...files]

  // As the file is generated in the temp directory we need to set the baseUrl
  // so everything resolvese correctly.
  tsConfig.compilerOptions.baseUrl = baseDir

  // Write the tsconfig to the gen directory.
  const tsConfigName = `lit-mangler-check-tsconfig_${files.map((file) => path.basename(file)).join('_')}.json`
  const tempTsConfigPath = path.join(genDir, tsConfigName)
  fs.writeFileSync(tempTsConfigPath, JSON.stringify(tsConfig, null, 2))
  return path.normalize(tempTsConfigPath)
}

const runTypecheck = (genDir: string, files: string[]) => {
  const result = childProcess.spawnSync('tsc', [
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

commander
  .command('mangle')
  .option(
    '-m, --mangler <file>',
    'The file with containing the mangler instructions',
  )
  .option('-i, --input <file>', 'The file to mangle')
  .option('-o, --output <file>', 'Where to output the mangled file')
  .option('-g, --gen-dir <folder>', 'The folder for generated files')
  .option('-t, --typecheck', 'Run typechecking before mangling')
  .action(
    async ({
      mangler,
      input,
      output,
      typecheck,
      genDir,
    }: {
      mangler: string
      input: string
      output: string
      typecheck: boolean
      genDir: string
    }) => {
      load(input)

      if (typecheck) {
        runTypecheck(genDir, [mangler])
      }

      // Note: Windows requires the file:// scheme for absolute import paths
      await import(`file://${mangler}`)
      write(output)
    },
  )

commander.parse(process.argv)
