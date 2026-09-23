// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { fileFragment, filePathFromHash } from './file_fragment'

describe('filePathFromHash', () => {
  it.each([
    ['#file=notes.txt', 'notes.txt'],
    ['#file=src/main.ts', 'src/main.ts'],
    // The fragment is percent-encoded, so names with spaces survive it.
    ['#file=my%20notes.txt', 'my notes.txt'],
    // A fragment can carry more than the file.
    ['#line=12&file=notes.txt', 'notes.txt'],
  ])('reads the file out of %s', (hash, expected) => {
    expect(filePathFromHash(hash)).toBe(expected)
  })

  it.each([
    ['', 'an empty fragment'],
    ['#', 'a bare hash'],
    ['#file=', 'an empty file'],
    ['#notes.txt', 'a fragment that names no parameter'],
    ['#line=12', 'a fragment about something else'],
  ])('asks for no file for %p (%s)', (hash) => {
    expect(filePathFromHash(hash)).toBeNull()
  })
})

describe('fileFragment', () => {
  it('asks for a file', () => {
    expect(fileFragment('notes.txt')).toBe('#file=notes.txt')
  })

  it('round-trips a path that needs encoding', () => {
    const path = 'my dir/a#b?c.txt'
    expect(fileFragment(path)).toBe('#file=my%20dir%2Fa%23b%3Fc.txt')
    expect(filePathFromHash(fileFragment(path))).toBe(path)
  })
})
