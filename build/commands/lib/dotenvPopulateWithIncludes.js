// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const Log = require('./logging')
const fs = require('fs')
const path = require('path')
const dotenv = require('dotenv')

// Reads the .env file and includes other .env files using the
// include_env=<path> directive.
function dotenvPopulateWithIncludes(processEnv, envPath) {
  function readEnvFile(filePath) {
    if (!fs.existsSync(filePath)) {
      Log.error(`Error loading .env (not found) from: ${filePath}`)
      process.exit(1)
    }

    const envContent = fs.readFileSync(filePath, 'utf8')
    const lines = envContent.split('\n')
    let result = ''

    lines.forEach((line) => {
      const includeEnvMatch = line.match(/^include_env=([^#]+)(?:#.*)?$/)
      if (includeEnvMatch) {
        const includePath = includeEnvMatch[1].trim()
        const resolvedPath = path.resolve(path.dirname(filePath), includePath)
        result += readEnvFile(resolvedPath)
      } else {
        result += line + '\n'
      }
    })

    return result
  }

  const finalEnvContent = readEnvFile(envPath)
  dotenv.populate(processEnv, dotenv.parse(finalEnvContent))
}

module.exports = dotenvPopulateWithIncludes
