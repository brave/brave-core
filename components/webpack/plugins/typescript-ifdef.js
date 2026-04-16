// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Custom TypeScript wrapper that preprocesses source files with ifdef
 * directives before TypeScript parses them. This ensures type-checking
 * happens on the preprocessed code.
 *
 * Usage: Set ts-loader's `compiler` option to this module path.
 */

const ts = require('typescript')
const path = require('path')
const { parse } = require('./ifdef-parse.js')

// Load build flags - this runs at module load time
let buildFlags = {}
if (process.env.ROOT_GEN_DIR) {
  const buildFlagsPath = path.join(process.env.ROOT_GEN_DIR, 'brave/build_flags.json')
  try {
    buildFlags = require(buildFlagsPath)
  } catch (e) {
    console.warn(`typescript-ifdef: Could not load build flags from ${buildFlagsPath}`)
  }
} else {
  console.warn('typescript-ifdef: ROOT_GEN_DIR environment variable not set')
}

// === Custom TypeScript sys ===

const originalReadFile = ts.sys.readFile.bind(ts.sys)

ts.sys.readFile = function(filePath, encoding) {
  const content = originalReadFile(filePath, encoding)
  if (content === undefined) {
    return undefined
  }

  // Only preprocess TypeScript/JavaScript files in the brave directory
  if (/\.(ts|tsx|js|jsx)$/.test(filePath) && filePath.includes('brave')) {
    try {
      return parse(content, buildFlags)
    } catch (e) {
      console.error(`typescript-ifdef: Error preprocessing ${filePath}:`, e)
      return content
    }
  }

  return content
}

// Re-export TypeScript
module.exports = ts
