/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getSearchEngineId } from './search_engine_id'

const engine = {
  prepopulateId: 0n,
  name: 'Custom search',
  keyword: 'custom',
  host: 'example.com',
  faviconUrl: '',
}

describe('getSearchEngineId', () => {
  it('preserves existing built-in engine preferences', () => {
    expect(getSearchEngineId({ ...engine, prepopulateId: 1n })).toBe(
      'example.com',
    )
    expect(getSearchEngineId({ ...engine, prepopulateId: 1n, host: '' })).toBe(
      '',
    )
  })

  it('distinguishes custom engines sharing a host', () => {
    const second = { ...engine, keyword: 'custom-images' }
    expect(getSearchEngineId(engine)).toBe('keyword:custom')
    expect(getSearchEngineId(second)).toBe('keyword:custom-images')
  })

  it('distinguishes a custom engine from a built-in on the same host', () => {
    expect(getSearchEngineId(engine)).not.toBe(
      getSearchEngineId({ ...engine, prepopulateId: 1n }),
    )
  })
})
