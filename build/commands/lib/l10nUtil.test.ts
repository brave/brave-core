// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import l10nUtil from './l10nUtil.ts'

let root: string

function write(
  relativePath: string,
  parts: string[],
  tag = 'grit-part',
): string {
  const filePath = path.join(root, relativePath)
  fs.mkdirSync(path.dirname(filePath), { recursive: true })
  const partTags = parts.map((part) => `<part file="${part}" />`).join('')
  fs.writeFileSync(filePath, `<${tag}>${partTags}</${tag}>`, 'utf8')
  return filePath
}

beforeEach(() => {
  root = fs.mkdtempSync(path.join(os.tmpdir(), 'l10n-util-'))
})

afterEach(() => {
  fs.rmSync(root, { recursive: true, force: true })
})

test('deletes the Brave copies of the parts upstream dropped', () => {
  const mapping = {
    [write('chromium/kept.grdp', [])]: write('brave/kept.grdp', []),
    [write('chromium/sub.grdp', [])]: write('brave/sub.grdp', ['dropped.grdp']),
    [write('chromium/other.grdp', [])]: write('brave/moved.grdp', []),
    // New upstream: no Brave copy yet.
    [write('chromium/new.grdp', [])]: path.join(root, 'brave/new.grdp'),
    [write('chromium/x.grd', ['kept.grdp', 'sub.grdp', 'new.grdp'], 'grit')]:
      write(
        'brave/x.grd',
        ['kept.grdp', 'sub.grdp', 'gone.grdp', 'moved.grdp'],
        'grit',
      ),
  }
  write('brave/dropped.grdp', [])
  write('brave/gone.grdp', ['gone_child.grdp'])
  write('brave/gone_child.grdp', [])

  const removed = l10nUtil.getRemovedGRDParts(mapping)
  expect([...removed.entries()]).toEqual([
    [path.join(root, 'brave/sub.grdp'), new Set(['dropped.grdp'])],
    [path.join(root, 'brave/x.grd'), new Set(['gone.grdp', 'moved.grdp'])],
  ])

  const deleted = l10nUtil.deleteRemovedGRDParts(removed, mapping)
  expect(deleted.sort()).toEqual(
    ['brave/dropped.grdp', 'brave/gone.grdp', 'brave/gone_child.grdp'].map(
      (p) => path.join(root, p),
    ),
  )
  // Still written by the mapping, from another upstream file.
  expect(fs.existsSync(path.join(root, 'brave/moved.grdp'))).toBe(true)
  expect(fs.existsSync(path.join(root, 'brave/kept.grdp'))).toBe(true)
})
