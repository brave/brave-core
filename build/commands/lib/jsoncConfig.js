// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const fs = require('fs')
const jsoncParser = require('jsonc-parser')
const util = require('./util')

class JsoncConfig {
  constructor(configPath, templateConfig = '{}') {
    this.configPath = configPath
    if (fs.existsSync(this.configPath)) {
      this.configText =
        fs.readFileSync(this.configPath, 'utf8') || templateConfig
      this.config = jsoncParser.parse(this.configText)
    } else {
      this.configText = templateConfig
      this.config = jsoncParser.parse(this.configText)
      this.#save()
    }
    freezeRecursive(this.config)
  }

  set(path, value) {
    if (!Array.isArray(path)) {
      path = path.split('.')
    }
    const edits = jsoncParser.modify(this.configText, path, value, {
      formattingOptions: {
        insertSpaces: true,
        tabSize: 2
      }
    })
    this.configText = jsoncParser.applyEdits(this.configText, edits)
    this.config = jsoncParser.parse(this.configText)
    freezeRecursive(this.config)
    this.#save()
  }

  #save() {
    util.writeFileIfModified(this.configPath, this.configText)
  }
}

function freezeRecursive(obj) {
  Object.freeze(obj)
  for (const value of Object.values(obj)) {
    if (value instanceof Object) {
      freezeRecursive(value)
    }
  }
}

module.exports = JsoncConfig
