// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { makeEntryFragment, parseEntryFragment } from './entry_fragment'

describe('entry fragment', () => {
  afterEach(() => {
    jest.restoreAllMocks()
  })

  it('names the entry', () => {
    expect(parseEntryFragment(makeEntryFragment('entry-1'))?.entryUuid).toBe(
      'entry-1',
    )
  })

  it('names an entry whose uuid needs escaping', () => {
    expect(parseEntryFragment(makeEntryFragment('a&b=c#d'))?.entryUuid).toBe(
      'a&b=c#d',
    )
  })

  it('differs for each use, so using the same entry again is noticed', () => {
    const now = jest.spyOn(Date, 'now')
    now.mockReturnValueOnce(1000)
    now.mockReturnValueOnce(1001)
    const first = makeEntryFragment('entry-1')
    const second = makeEntryFragment('entry-1')
    expect(first).not.toBe(second)
    expect(parseEntryFragment(first)?.entryUuid).toBe(
      parseEntryFragment(second)?.entryUuid,
    )
    expect(parseEntryFragment(first)?.nonce).not.toBe(
      parseEntryFragment(second)?.nonce,
    )
  })

  it('finds no entry in anything else', () => {
    expect(parseEntryFragment('')).toBeUndefined()
    expect(parseEntryFragment('#')).toBeUndefined()
    expect(parseEntryFragment('#other=1')).toBeUndefined()
    expect(parseEntryFragment('#entry=')).toBeUndefined()
    expect(parseEntryFragment('#entry=&n=1')).toBeUndefined()
  })
})
