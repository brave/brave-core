// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { pathToFileURL } from 'node:url'

test('provides CommonJS equivalents for import.meta', () => {
  expect(import.meta.main).toBe(false)
  expect(import.meta.url).toBe(pathToFileURL(__filename).href)
  expect(import.meta.resolve('typescript')).toBe(
    pathToFileURL(require.resolve('typescript')).href,
  )
  expect(import.meta.resolve?.('typescript')).toBe(
    pathToFileURL(require.resolve('typescript')).href,
  )
  expect(import.meta.dirname).toBe(__dirname)
  expect(import.meta.filename).toBe(__filename)
})
