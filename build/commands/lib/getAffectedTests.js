// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const { promisify } = require('util')
const { readFile, writeFile } = require('fs/promises')
const exec = promisify(require('child_process').execFile)
const path = require('path')
const config = require('../lib/config')

const getTestTargets = (outDir, filters = ['//*']) =>
  exec('./vendor/depot_tools/gn', [
    'ls',
    outDir,
    '--type=executable',
    '--testonly=true',
    filters,
  ]).then((x) => x.stdout.trim().split('\n'))

// set base = HEAD if you want to ignore the current workspace changes
async function getModifiedFiles(target = 'HEAD~', base = null) {
  const args = ['diff', '--name-only', target, base].filter((x) => x)

  return exec('git', args, { maxBuffer: 1024 * 1024 * 50 }).then((x) =>
    x.stdout
      .trim()
      .split('\n')
      .filter((x) => x)
      .map((x) => '//brave/' + x),
  )
}

async function getReferenceCommit() {
  if (process.env.GIT_PREVIOUS_SUCCESSFUL_COMMIT) {
    return process.env.GIT_PREVIOUS_SUCCESSFUL_COMMIT
  }

  const currentBranch = await exec('git', [
    'rev-parse',
    '--abbrev-ref',
    'HEAD',
  ]).then((x) => x.stdout)

  if (currentBranch !== 'master') {
    return 'origin/master'
  }

  // bail: we don't know the last succesful test run
  return null
}

async function getAffectedTests(outDir, filters = ['//*']) {
  // JENKINS sets GIT_PREVIOUS_SUCCESSFUL_COMMIT
  // TODO: find TeamCity equivalent.
  // TODO: we can optimize further by getting the last failure commit
  const targetCommit = await getReferenceCommit()
  if (!targetCommit) {
    return null
  }

  console.log('analyzing tests based on', targetCommit)

  const root = path.resolve(process.cwd(), '../')
  outDir =
    outDir.startsWith('..') || outDir.startsWith('/')
      ? outDir
      : `${root}/${outDir}`
  const testTargets = await getTestTargets(outDir, filters)
  const files = await getModifiedFiles(targetCommit)

  const toAnalyze = {
    files,
    test_targets: testTargets,
  }

  // paths are relative to package.json
  await writeFile(
    `${root}/out/analyze.json`,
    JSON.stringify(toAnalyze, null, 2),
    'utf-8',
  )
  await exec('./vendor/depot_tools/gn', [
    'analyze',
    outDir,
    `${root}/out/analyze.json`,
    `${root}/out/out.json`,
  ])

  const output = await readFile(`${root}/out/out.json`, 'utf-8').then(
    JSON.parse,
  )

  return {
    outDir,
    filters,
    ...toAnalyze,
    targetCommit,
    affectedTests: output.test_targets,
  }
}

module.exports = getAffectedTests
