// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Outputs a file containing all files that were used in
// the compilation, to be used by the ninja build system.
// This can be useful to know which files to monitor for changes
// to cause a re-build only when neccessary.

const path = require('path')
const fs = require('fs')
const mkdirp = require('mkdirp')

function generateDepfileContent(outputName, depPaths) {
  // File format is "dependency information in the syntax of a Makefile"
  // (https://ninja-build.org/manual.html#_depfile)
  return `${outputName}: ${depPaths.join(' ')}`
}

function writeDepfileContentSync(filePath, content) {
  mkdirp.sync(path.dirname(filePath))
  fs.writeFileSync(filePath, content, { encoding: 'utf8' })
}

class GenerateDepfilePlugin {
  constructor (options) {
    this.options = {
      depfilePath: 'depfile.d',
      depfileSourceName: '[UnknownOutputName]',
      ...options
    }
  }

  apply (compiler) {
    // These hooks cannot be used async, so must do sync ops.
    compiler.hooks.compilation.tap(this.constructor.name, (compilation) => {
      compilation.hooks.finishModules.tap(this.constructor.name, (modules) => {
        const absoluteDepsPaths = modules.map(module => module.resource)
        const depfileContent = generateDepfileContent(this.options.depfileSourceName, absoluteDepsPaths)
        writeDepfileContentSync(this.options.depfilePath, depfileContent)
      })
    })
  }
}

module.exports = GenerateDepfilePlugin
