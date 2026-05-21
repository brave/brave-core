// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import build from './build.js'
import config from './config.ts'
import fs from 'fs-extra'
import path from 'node:path'
import { spawn } from 'node:child_process'
import jszip from 'jszip'

const fuzzerBuildConfig = 'Fuzzer'

const buildFuzzer = (fuzzerTestTarget, options) => {
  options.use_libfuzzer = true
  options.is_asan = true
  options.target = fuzzerTestTarget
  options.is_component_build = false

  build(fuzzerBuildConfig, options)
}

const getBinary = (suite) => {
  return process.platform === 'win32' ? `${suite}.exe` : suite
}

const unzip = (zipFile, outdir) => {
  fs.readFile(zipFile, (err, data) => {
    if (err) {
      throw err
    }
    jszip.loadAsync(data).then((zip) => {
      // Sensitive
      zip.forEach((_, zipEntry) => {
        const resolvedPath = path.join(outdir, zipEntry.name)
        if (!zip.file(zipEntry.name)) {
          if (!fs.existsSync(resolvedPath)) {
            fs.mkdirSync(resolvedPath)
          }
        } else {
          // @ts-ignore
          zip
            .file(zipEntry.name)
            .async('nodebuffer')
            .then((content) => {
              if (!fs.existsSync(resolvedPath)) {
                fs.mkdirSync(path.dirname(resolvedPath))
              }
              fs.writeFileSync(resolvedPath, content)
            })
        }
      })
    })
  })
}

const runFuzzer = (passthroughArgs, suite) => {
  config.buildConfig = fuzzerBuildConfig
  config.update({})

  let fuzzerArgs = []

  const dictFile = path.join(config.outputDir, suite + '.dict')
  if (fs.existsSync(dictFile)) {
    fuzzerArgs.push('-dict=' + dictFile)
  }

  const seedCorpusFile = path.join(config.outputDir, suite + '_seed_corpus.zip')

  if (fs.existsSync(seedCorpusFile)) {
    const seedCorpus = path.join(config.outputDir, suite)
    fuzzerArgs.push(seedCorpus)
    unzip(seedCorpusFile, seedCorpus)
  }

  fuzzerArgs = fuzzerArgs.concat(passthroughArgs)

  console.log('Running ' + getBinary(suite) + ' ' + fuzzerArgs)

  spawn(path.join(config.outputDir, getBinary(suite)), fuzzerArgs, {
    stdio: 'inherit',
  })
}

export { buildFuzzer, runFuzzer }
