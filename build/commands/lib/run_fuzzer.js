// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const config = require('../lib/config')
const path = require('path')
const fs = require('fs-extra')
const { spawn } = require('child_process')
const unzip = require('jszip-unzip').default
const util = require('../lib/util')

const getBinary = (suite) => {
  return (process.platform === 'win32') ? `${suite}.exe` : suite
}

const run_fuzzer = (suite) => {
  options = { C: 'Fuzzer' }
  config.buildConfig = 'Fuzzer'
  config.update(options)

  let fuzzerArgs = []

  const dict_file = path.join(config.outputDir, suite + '.dict')
  if (fs.existsSync(dict_file)) {
    fuzzerArgs.push('-dict=' + dict_file)
  }

  const seed_corpus_file = path.join(config.outputDir, suite + '_seed_corpus.zip')

  if (fs.existsSync(seed_corpus_file)) {
    const seed_corpus = path.join(config.outputDir, suite)
    fuzzerArgs.push(seed_corpus)
    unzip(fs.readFileSync(seed_corpus_file), { to: seed_corpus })
  }

  spawn(path.join(config.outputDir, getBinary(suite)), fuzzerArgs, { stdio: "inherit" })
}

module.exports = run_fuzzer
