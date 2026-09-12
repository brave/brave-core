// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Command } from 'commander'
import { collect, parseInteger } from '../lib/commandsUtils.ts'

import assert from 'node:assert'
import crypto from 'node:crypto'
import fs from 'node:fs'
import path from 'node:path'
import config from '../lib/config.ts'
import { comparePak } from '../lib/pakDiff.ts'

export const command = new Command('compare_build_artifacts')
  .command('compare_build_artifacts')
  .description(
    'Compare artifacts from two separately built checkouts for determinism.',
  )
  .argument('<build_dir_1>', 'first build output directory')
  .argument('<build_dir_2>', 'second build output directory')
  .option('--artifact <name>', 'artifact to compare (repeatable)', collect, [
    'brave_resources.pak',
  ])
  .option(
    '--max_diff_lines <n>',
    'maximum lines printed per diff',
    parseInteger,
    200,
  )
  .option('--no_prettier', 'do not normalize text entries with prettier')
  .option('--warn_only', 'report differences without failing')
  .action(async (dir1: string, dir2: string, options) => {
    const firstDir = resolveBuildDir(dir1)
    const secondDir = resolveBuildDir(dir2)
    assert(
      fs.statSync(firstDir).isDirectory(),
      `${firstDir} is not a directory`,
    )
    assert(
      fs.statSync(secondDir).isDirectory(),
      `${secondDir} is not a directory`,
    )
    assert.notStrictEqual(
      fs.realpathSync(firstDir),
      fs.realpathSync(secondDir),
      'Build directories resolve to the same tree',
    )

    let differences = 0
    for (const artifact of options.artifact) {
      const first = path.join(firstDir, artifact)
      const second = path.join(secondDir, artifact)
      assert(fs.existsSync(first), `${first} does not exist`)
      assert(fs.existsSync(second), `${second} does not exist`)
      const sameSize = fs.statSync(first).size === fs.statSync(second).size
      const [firstHash, secondHash] = sameSize
        ? await Promise.all([sha256(first), sha256(second)])
        : ['', '']
      if (sameSize && firstHash === secondHash) {
        console.log(`${artifact}: identical`)
        continue
      }
      differences++
      console.log(`${artifact}: differs`)
      if (artifact.endsWith('.pak')) {
        const result = await comparePak(first, second, firstDir, secondDir, {
          maxDiffLines: options.max_diff_lines,
          prettier: !options.no_prettier,
        })
        console.log(
          `${artifact}: added ${result.added.length}, removed ${result.removed.length}, changed ${result.changed.length}`,
        )
        if (result.added.length)
          console.log(`added ids: ${result.added.join(', ')}`)
        if (result.removed.length)
          console.log(`removed ids: ${result.removed.join(', ')}`)
        console.log(result.diffs.join('\n'))
        if (result.metadataChanged) {
          console.log(`${artifact}: pak print output differs`)
        }
      } else {
        console.log(
          `size: ${fs.statSync(first).size} vs ${fs.statSync(second).size}`,
        )
        const [firstHash, secondHash] = await Promise.all([
          sha256(first),
          sha256(second),
        ])
        console.log(`sha256: ${firstHash} vs ${secondHash}`)
      }
    }
    console.log(
      `Compared ${options.artifact.length} artifact(s): ${differences} differed`,
    )
    if (differences && !options.warn_only) process.exitCode = 1
  })

function resolveBuildDir(directory: string) {
  return path.resolve(
    path.isAbsolute(directory)
      ? directory
      : path.join(config.srcDir, 'out', directory),
  )
}

async function sha256(file: string) {
  const hash = crypto.createHash('sha256')
  for await (const chunk of fs.createReadStream(file)) {
    hash.update(chunk)
  }
  return hash.digest('hex')
}
