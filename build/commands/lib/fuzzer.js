// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const build = require('./build')
const config = require('../lib/config')
const fs = require('fs-extra')
const path = require('path')
const { spawn } = require('child_process')
const unzip = require('jszip-unzip').default

const fuzzerBuildConfig = 'Fuzzer'

const buildFuzzer = (fuzzer_test_target, options) => {
  options.use_libfuzzer = true
  options.is_asan = true
  options.target = fuzzer_test_target
  options.is_component_build = false

  build(fuzzerBuildConfig, options)
}

const getBinary = (suite) => {
  return (process.platform === 'win32') ? `${suite}.exe` : suite
}

const runFuzzer = (passthroughArgs, suite) => {
  config.buildConfig = fuzzerBuildConfig
  config.update({})

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

module.exports = { buildFuzzer, runFuzzer }
