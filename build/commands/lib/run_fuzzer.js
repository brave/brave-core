// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('../lib/config')
const path = require('path')
const fs = require('fs-extra')
const { spawn } = require('child_process')
const unzip = require('jszip-unzip').default

const getBinary = (suite) => {
  return (process.platform === 'win32') ? `${suite}.exe` : suite
}

const runFuzzer = (passthroughArgs, suite) => {
  options = { C: 'Fuzzer' }
  config.buildConfig = 'Fuzzer'
  config.update(options)

  let fuzzerArgs = []

  const dictFile = path.join(config.outputDir, suite + '.dict')
  if (fs.existsSync(dictFile)) {
    fuzzerArgs.push('-dict=' + dictFile)
  }

  const seedCorpusFile =
    path.join(config.outputDir, suite + '_seed_corpus.zip')

  if (fs.existsSync(seedCorpusFile)) {
    const seedCorpus = path.join(config.outputDir, suite)
    fuzzerArgs.push(seedCorpus)
    unzip(fs.readFileSync(seedCorpusFile), { to: seedCorpus })
  }

  fuzzerArgs = fuzzerArgs.concat(passthroughArgs)

  console.log('Running ' + getBinary(suite) + ' ' + fuzzerArgs)

  spawn(path.join(config.outputDir, getBinary(suite)), fuzzerArgs, {
    stdio: "inherit"
  })
}

module.exports = runFuzzer
