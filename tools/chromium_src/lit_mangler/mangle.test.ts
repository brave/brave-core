// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'fs'
import path from 'path'
import * as diff from 'diff'
import { genPath, outputPath } from '../../../build/commands/lib/guessConfig'

// Note: Headers are stripped during preprocess, we're not interested in them
// showing up in the snapshot.
const header = `// Copyright XXXX The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

`

const root = path.resolve(__dirname, '..', '..', '..')
const chromiumSrc = path.join(root, 'chromium_src')
function* walkManglers(root = chromiumSrc) {
  for (const child of fs.readdirSync(root)) {
    const childPath = path.join(root, child)
    if (fs.statSync(childPath).isDirectory()) {
      for (const match of walkManglers(childPath)) {
        yield match
      }
    } else if (childPath.endsWith('.lit_mangler.ts')) {
      yield childPath
    }
  }
}

// Sometimes mangled files are not in a directory corresponding to the source
// file. This function tries to find the mangled file by traversing up the
// directory structure and inserting '/preprocessed/' at various positions.
// https://github.com/brave/brave-browser/issues/46817
function findMangledFileAndOriginalFile(
  manglerPath: string,
  genPath: string,
  root: string,
): { mangledPath: string; originalPath: string } {
  const baseName = path.basename(manglerPath)
  const mangledPathWithoutProcessed = path.relative(
    root,
    path.join(genPath, manglerPath),
  )
  const pathParts = path
    .relative(genPath, path.resolve(mangledPathWithoutProcessed))
    .split(path.sep)

  const assumedMangledPath = path.relative(
    root,
    path.join(
      genPath,
      manglerPath.replace(baseName, 'preprocessed/' + baseName),
    ),
  )
  let mangledPath = assumedMangledPath
  const originalPath = path.join('..', manglerPath)

  let attempts = 0
  while (attempts < pathParts.length - 1 && !fs.existsSync(mangledPath)) {
    // Try inserting 'preprocessed' at position
    // e.g.
    // iter 1) a/b/preprocessed/c/file
    // iter 2) a/preprocessed/b/c/file
    const insertPosition = Math.max(0, pathParts.length - 2 - attempts)
    const newPathParts = [
      ...pathParts.slice(0, insertPosition),
      'preprocessed',
      ...pathParts.slice(insertPosition),
    ]

    mangledPath = path.join(genPath, ...newPathParts)

    attempts++
  }
  if (!fs.existsSync(mangledPath)) {
    throw new Error(`Mangled file not found: ${assumedMangledPath}`)
  }

  return { mangledPath, originalPath }
}

describe('mangled files should have up to date snapshots', () => {
  for (const mangler of walkManglers()) {
    const name = path.relative(root, mangler).replaceAll(path.sep, '/')
    it(`./${name} should match snapshot`, () => {
      const manglerPath = name
        .replace('.lit_mangler.ts', '')
        .replace(`chromium_src/`, '')

      const { mangledPath, originalPath } = findMangledFileAndOriginalFile(
        manglerPath,
        genPath,
        root,
      )

      const mangledText = fs
        .readFileSync(mangledPath, 'utf8')
        .replaceAll('\r\n', '\n')
      const originalText = fs
        .readFileSync(originalPath, 'utf8')
        .replaceAll('\r\n', '\n')
        .substring(header.length)
      const linesDiff = diff.createTwoFilesPatch(
        originalPath.replaceAll(path.sep, '/'),
        path.relative(outputPath, mangledPath).replaceAll(path.sep, '/'),
        originalText,
        mangledText,
      )
      expect(linesDiff).toMatchSnapshot()
    })
  }
})
