// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const assert = require('assert')
const config = require('./config')
const fs = require('fs')
const path = require('path')
const util = require('./util')

// This function is used to patch build/config/unsafe_buffers_paths.txt, to
// work around the issue that redirect_cc ends up prepending all file paths with
// `src/`, which breaks the filter used by the chromium clang plugin.
// With this function, all entries are duplicated at the end with `src` added
// to them.
function updateUnsafeBuffersPaths() {
  const braveVersionParts = config.braveVersion.split('.')
  assert(braveVersionParts.length == 3)


  // the absolute path for the file to be patched.
  const unsafeBuffersPath =
      path.join(config.srcDir, 'build', 'config', 'unsafe_buffers_paths.txt')

  // The files contents.
  const bufferPaths = fs.readFileSync(unsafeBuffersPath).toString()

  // txt files use eol=lf in .gitattributes.
  const bufferPathLines = bufferPaths.split('\n')

  updatedPathLines = bufferPathLines.filter(
      (line) => line.startsWith('-') || line.startsWith('+'))
  updatedPathLines = updatedPathLines.map(
      (line) => {return line.slice(0, 1) + 'src/' + line.slice(1)})

  updatedContents = [
    ...bufferPathLines,
    '# Appended path correction for redirect_cc by update_patches', '-brave/',
    '-src/brave', ...updatedPathLines, ''
  ]
  fs.writeFileSync(unsafeBuffersPath, updatedContents.join('\n'))
}

module.exports = updateUnsafeBuffersPaths
