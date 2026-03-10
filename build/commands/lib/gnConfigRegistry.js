// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

// Registry for graph-based GN configs. Used by configs.js and gnConfigResolver.
// Configs and rules are stored here; resolution is done by gnConfigResolver.

const configs = new Map()
const rules = []

function gnConfig (name, opts = {}) {
  if (configs.has(name)) {
    throw new Error(`Duplicate gnConfig: ${name}`)
  }
  configs.set(name, { name, ...opts })
}

function gnRule (description, validator) {
  rules.push({ description, validator })
}

function getConfigs () {
  return configs
}

function getRules () {
  return rules
}

module.exports = {
  gnConfig,
  gnRule,
  getConfigs,
  getRules,
}
