// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import { prefixPatchPaths } from './util.js'

/** @typedef {{path?: string, patchPath: string}} PatchStatus */

describe('prefixPatchPaths', function () {
  test('leaves a status carrying no path alone', function () {
    // A patch too malformed to read the files it applies to reports no path,
    // which used to be joined all the same, failing the whole apply with
    // `ERR_INVALID_ARG_TYPE` instead of reporting the patch that broke.
    /** @type {PatchStatus} */
    const status = { patchPath: '/patches/broken.patch' }

    expect(() =>
      prefixPatchPaths([status], 'third_party', 'devtools-frontend', 'src'),
    ).not.toThrow()
    expect(status.path).toBeUndefined()
  })

  test('prefixes the repo onto a status carrying a path', function () {
    /** @type {PatchStatus} */
    const status = {
      patchPath: '/patches/scripts-build-ts_library.py.patch',
      path: path.join('scripts', 'build', 'ts_library.py'),
    }

    prefixPatchPaths([status], 'third_party', 'devtools-frontend', 'src')

    expect(status.path).toBe(
      path.join(
        'third_party',
        'devtools-frontend',
        'src',
        'scripts',
        'build',
        'ts_library.py',
      ),
    )
  })

  test('prefixes only the statuses carrying a path', function () {
    /** @type {PatchStatus} */
    const withPath = { patchPath: '/patches/file1.patch', path: 'file1' }
    /** @type {PatchStatus} */
    const withoutPath = { patchPath: '/patches/broken.patch' }

    prefixPatchPaths([withPath, withoutPath], 'v8')

    expect(withPath.path).toBe(path.join('v8', 'file1'))
    expect(withoutPath.path).toBeUndefined()
  })

  test('handles a repo with no patches at all', function () {
    expect(() => prefixPatchPaths([], 'third_party', 'ffmpeg')).not.toThrow()
  })
})
