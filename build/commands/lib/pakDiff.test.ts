// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  classifyEntries,
  isBinaryContent,
  normalizeText,
  resourceFileName,
  truncateDiff,
} from './pakDiff.ts'

describe('pakDiff', () => {
  it('detects binary content and invalid UTF-8', () => {
    expect(isBinaryContent(Buffer.from([0]))).toBe(true)
    expect(isBinaryContent(Buffer.from([0xff]))).toBe(true)
    expect(isBinaryContent(Buffer.from('text'))).toBe(false)
  })

  it('classifies added, removed, and changed entries', () => {
    expect(
      classifyEntries(
        new Map([
          ['removed', Buffer.from('a')],
          ['changed', Buffer.from('a')],
        ]),
        new Map([
          ['changed', Buffer.from('b')],
          ['added', Buffer.from('c')],
        ]),
      ),
    ).toEqual({ added: ['added'], removed: ['removed'], changed: ['changed'] })
  })

  it('truncates long diffs', () => {
    expect(truncateDiff('a\nb\nc\nd', 2)).toBe('a\nb\n... 2 more lines')
  })

  it('truncates long individual lines with an explicit marker', () => {
    const diff = `prefix ${'x'.repeat(300)}`
    const output = truncateDiff(diff, 10)
    expect(output).toContain('[line truncated]')
    expect(output.length).toBeLessThan(300)
  })

  it('infers a formatter extension from the resource ID', () => {
    expect(resourceFileName('IDR_BRAVE_NEW_TAB_BUNDLE')).toBe(
      'IDR_BRAVE_NEW_TAB_BUNDLE.js',
    )
    expect(
      resourceFileName(
        'IDR_ON_DEVICE_SPEECH_RECOGNITION_WORKER_SPEECH_WORKER_BUNDLE_JS',
      ),
    ).toBe('IDR_ON_DEVICE_SPEECH_RECOGNITION_WORKER_SPEECH_WORKER_BUNDLE_JS.js')
  })

  it('normalizes text through the selected formatter', async () => {
    const formatter = async (content: string) =>
      content
        .replace(/\s*=\s*/g, '=')
        .replace(/\s+/g, ' ')
        .trim()
    await expect(
      normalizeText(Buffer.from('const x=1;'), 'resource.js', formatter),
    ).resolves.toBe('const x=1;')
    await expect(
      normalizeText(Buffer.from('const  x = 1;'), 'resource.js', formatter),
    ).resolves.toBe('const x=1;')
  })
})
