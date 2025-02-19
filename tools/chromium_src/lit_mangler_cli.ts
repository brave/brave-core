// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import commander from 'commander'
import { load, write } from './lit_mangler'
import childProcess from 'child_process'
import path from 'path'
import fs from 'fs'
import os from 'os'

const baseDir = path.join(__dirname, '../../')
const tsConfigPath = path.join(baseDir, 'tsconfig-mangle.json')

/**
 * Unfortunately tsc doesn't support passing in a path map for a single file
 * check, so we have to generate a new tsconfig based on tsconfig-mangle.json
 * and the file we want to check.
 * @param file The file to generate the tsconfig for
 * @returns The path to the generated tsconfig
 */
export const getTsConfigForSingleFileCheck = (file: string) => {
    const tsConfig = JSON.parse(fs.readFileSync(tsConfigPath, 'utf8'))

    // Override the include path to only include the lit_mangler and the file
    // we want to check
    tsConfig.include = [
        path.join(__dirname, 'lit_mangler.ts'),
        file
    ]

    // As the file is generated in the temp directory we need to set the baseUrl
    // so everything resolvese correctly.
    tsConfig.compilerOptions.baseUrl = baseDir

    // Write the tsconfig to a temporary file.
    const tempDir = os.tmpdir()
    const tempTsConfigPath = path.join(tempDir, `${path.basename(file)}-tsconfig.json`)
    fs.writeFileSync(tempTsConfigPath, JSON.stringify(tsConfig, null, 2))
    return tempTsConfigPath
}

commander
    .command('mangle')
    .option('-m, --mangler <file>', 'The file with containing the mangler instructions')
    .option('-i, --input <file>', 'The file to mangle')
    .option('-o, --output <file>', 'Where to output the mangled file')
    .option('-t, --typecheck', 'Run typechecking before mangling')
    .action(async ({ mangler, input, output, typecheck }: { mangler: string, input: string, output: string, typecheck: boolean }) => {
        load(input)

        if (typecheck) {
            const result = childProcess.spawnSync('tsc', ['-p', getTsConfigForSingleFileCheck(mangler)])
            if (result.status !== 0) {
                console.error('Typechecking failed:\n', result.stdout.toString())
                process.exit(1)
            }
        }

        await import(mangler)
        write(output)
    })

commander
    .command('typecheck')
    .action(async () => {
        const result = childProcess.spawnSync('tsc', ['--project', tsConfigPath])
        if (result.status !== 0) {
            console.error('Typechecking failed:\n', result.stdout.toString())
            process.exit(1)
        }
    })

commander.parse(process.argv)
