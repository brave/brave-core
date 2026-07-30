// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { spawnSync } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'
import process from 'node:process'
import { Argument, Command } from 'commander'

const storybookConfigDir = import.meta.dirname
const braveCoreDir = path.resolve(storybookConfigDir, '../..')
const storybookCli = path.join(
  braveCoreDir,
  'node_modules',
  'storybook',
  'bin',
  'index.cjs',
)

export const storybookCommand = new Command('storybook')
  .description('Run Storybook (dev or static build)')
  .addArgument(
    new Argument('<mode>', 'Storybook mode').choices(['dev', 'build']),
  )
  .requiredOption(
    '--root-gen-dir <path>',
    'Path to the build gen directory (out/<config>/gen)',
  )
  .option(
    '--output-dir <path>',
    'Static Storybook output directory (required for build mode)',
  )
  .allowExcessArguments(true)
  .allowUnknownOption(true)
  .action((mode, options, program) => {
    const rootGenDir = path.resolve(options.rootGenDir)
    const outputDir = options.outputDir
      ? path.resolve(options.outputDir)
      : undefined

    if (mode === 'build' && !outputDir) {
      program.error('--output-dir is required when --mode=build')
    }

    if (!fs.existsSync(rootGenDir)) {
      program.error(
        `Failed to find build output 'gen' folder at '${rootGenDir}'. `
          + 'Have you run a brave-core build yet with the specified '
          + '(or default) configuration?',
      )
    }

    const nodeArgs = [
      '--max-old-space-size=8192',
      storybookCli,
      mode,
      '-c',
      storybookConfigDir,
    ]

    if (mode === 'build' && outputDir) {
      nodeArgs.push('-o', outputDir)
    }

    // Pass through any additional arguments.
    nodeArgs.push(...program.args)

    const env: NodeJS.ProcessEnv = {
      ...process.env,
      ROOT_GEN_DIR: rootGenDir,
    }

    console.log(`Using gen directory: ${rootGenDir}`)

    const result = spawnSync(process.execPath, nodeArgs, {
      env,
      stdio: 'inherit',
      cwd: braveCoreDir,
    })

    if (result.status !== 0) {
      process.exit(result.status ?? 1)
    }
  })
