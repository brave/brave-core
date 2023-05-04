// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const config = require('../lib/config')

// Helper to convert an environment variable value into a string.
function envValToString(envVal) {
  return envVal === undefined ? '' : envVal.toString()
}

// Gather all environment variable names.
const envKeys = new Set()
Object.keys(process.env).forEach((item) => envKeys.add(item))
Object.keys(config.defaultOptions.env).forEach((item) => envKeys.add(item))

// Compare current environment with a required one and output updated values.
for (let key of envKeys) {
  const newEnvVal = envValToString(config.defaultOptions.env[key])
  if (envValToString(process.env[key]) !== newEnvVal) {
    process.stdout.write(`${key}=${newEnvVal}\n`)
  }
}
