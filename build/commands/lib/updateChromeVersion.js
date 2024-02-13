// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const assert = require('assert')
const config = require('./config')
const fs = require('fs')
const path = require('path')
const Log = require('./logging')

function updateChromeVersion() {
  const braveVersionParts = config.braveVersion.split('.')
  assert(braveVersionParts.length == 3)

  const versionFilePath = path.join(config.srcDir, 'chrome', 'VERSION')
  const versionFileContent = fs.readFileSync(versionFilePath).toString()

  // chrome/VERSION eol=lf in .gitattributes.
  const versionLines = versionFileContent.split('\n')
  // There's an empty line after version lines, so expect at least 4 lines.
  assert(versionLines.length >= 4)

  // Match all VERSION file lines (MAJOR=123, MINOR=0, etc.).
  const versionLineRegex = /^(MAJOR|MINOR|BUILD|PATCH)=(\d+)$/
  for (let line = 0; line < 4; ++line) {
    assert(
      versionLines[line].search(versionLineRegex) == 0,
      `${versionLines[line]} (${line}) doesn't match ${versionLineRegex}`
    )
    if (line == 0) {
      // Keep MAJOR.
      assert(versionLines[line].startsWith('MAJOR='))
    } else {
      // Set MINOR, BUILD, PATCH to Brave version.
      versionLines[line] = versionLines[line].replace(
        versionLineRegex,
        `$1=${braveVersionParts[line - 1]}`
      )
    }
  }

  const versionToLog = versionLines
    .filter((n) => n)
    .join('.')
    .replace(/[A-Z=]/g, '')
  const newVersionFileContent = versionLines.join('\n')
  const patchNumberInfo = config.ignorePatchVersionNumber
    ? ' (PATCH=0 in dev builds)'
    : ''
  if (newVersionFileContent !== versionFileContent) {
    fs.writeFileSync(versionFilePath, newVersionFileContent)
    Log.status(`Set chrome/VERSION: ${versionToLog}${patchNumberInfo}`)
  } else {
    console.log(`chrome/VERSION: ${versionToLog}${patchNumberInfo}`)
  }
}

module.exports = updateChromeVersion
