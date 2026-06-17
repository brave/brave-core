// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Preloaded via `node --import` to track module loads and write a dep file.

import assert from 'node:assert'
import fs from 'node:fs'
import module from 'node:module'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { isMainThread } from 'node:worker_threads'

// Only register the tracking logic if it is the main thread (not a worker).
if (isMainThread) {
  // Root build directory.
  const rootBuildDir = process.env.DEPS_HOOK_ROOT_BUILD_DIR as string
  // Dep file to write.
  const depFilePath = process.env.DEPS_HOOK_DEP_FILE as string
  // Stamp file to write.
  const stampFilePath = process.env.DEPS_HOOK_STAMP_FILE as string
  // When `DEPS_HOOK_APPEND_DEPS` is set, depfile is appended.
  const appendDeps = process.env.DEPS_HOOK_APPEND_DEPS === '1'

  // Validate configuration.
  assert(rootBuildDir, 'Missing root build directory')
  assert(depFilePath, 'Missing dep file path')
  assert(stampFilePath, 'Missing stamp file path')
  assert.notStrictEqual(
    depFilePath,
    stampFilePath,
    'Dep file path must not equal to the stamp file path',
  )

  // Tracked dependencies.
  const deps = new Set<string>()

  // Remove the existing depfile.
  if (fs.existsSync(depFilePath)) {
    fs.unlinkSync(depFilePath)
  }

  // Register a hook to track module loads.
  module.registerHooks({
    load(url, context, nextLoad) {
      trackFileUrl(url)
      return nextLoad(url, context)
    },
  })

  // Register a hook to write the depfile and the stamp file on exit.
  process.on('exit', writeDepAndStampFiles)

  // ---------------------------------------------------------------------------
  // Helper functions
  // ---------------------------------------------------------------------------

  // Track a module load in deps set.
  function trackFileUrl(url: string) {
    if (!url.startsWith('file://')) {
      return
    }

    const relativePath = path.relative(rootBuildDir, fileURLToPath(url))
    deps.add(processDepfilePath(relativePath))
  }

  // Write the depfile and the stamp file.
  function writeDepAndStampFiles() {
    if (appendDeps && !fs.existsSync(depFilePath)) {
      // If deps are expected to be appended, but the depfile does not exist, do
      // nothing. The error will be caught by the parent build tool.
      return
    }

    if (!appendDeps) {
      assert(
        !fs.existsSync(depFilePath),
        'Dep file must not exist when not appending deps',
      )
    }

    // Write a stamp file to be used as an output of the depfile target.
    fs.mkdirSync(path.dirname(stampFilePath), { recursive: true })
    if (!fs.existsSync(stampFilePath)) {
      fs.writeFileSync(stampFilePath, '')
    }

    // Write the depfile.
    fs.mkdirSync(path.dirname(depFilePath), { recursive: true })
    const stampSection = formatDepfileStampSection()
    if (appendDeps) {
      fs.appendFileSync(depFilePath, `\n\n${stampSection}`)
    } else {
      fs.writeFileSync(depFilePath, stampSection)
    }
  }

  // Format the depfile section for the stamp file and tracked dependencies.
  function formatDepfileStampSection(): string {
    let depfileContent = ''
    depfileContent += `${processDepfilePath(stampFilePath)}:`
    for (const dep of deps) {
      depfileContent += ` \\\n ${dep}`
    }
    depfileContent += '\n'
    return depfileContent
  }

  // Ensure the dependency path is normalized and escaped for the depfile.
  function processDepfilePath(depPath: string): string {
    if (path.isAbsolute(depPath)) {
      throw new Error(`Found abs path in depfile: ${depPath}`)
    }
    let normalizedPath = depPath
    // Normalize the path separator to POSIX format.
    if (path.sep !== path.posix.sep) {
      normalizedPath = normalizedPath.replaceAll(path.sep, path.posix.sep)
    }
    return normalizedPath.replace(/ /g, '\\ ')
  }
}
