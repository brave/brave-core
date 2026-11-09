/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  getEnabledSearchEngineIds,
  getSelectedSearchEngineId,
} from './search_engine_selection'

const brave = { prepopulateId: 1n, host: 'search.brave.com', keyword: ':br' }
const custom = { prepopulateId: 0n, host: 'example.com', keyword: 'web' }
const engines = [brave, custom]

describe('NTP search selection', () => {
  it('restores enabled custom selections independently of host', () => {
    expect(
      getSelectedSearchEngineId(
        engines,
        new Set(['keyword:web']),
        'keyword:web',
        brave.host,
      ),
    ).toBe('keyword:web')
  })

  it('drops deleted or renamed selections and chooses an available fallback', () => {
    expect(
      getSelectedSearchEngineId(
        engines,
        new Set(['keyword:old']),
        'keyword:old',
        brave.host,
      ),
    ).toBe(brave.host)
    expect(
      getSelectedSearchEngineId(
        [custom],
        new Set(['keyword:old']),
        'keyword:old',
        brave.host,
      ),
    ).toBe('keyword:web')
  })

  it('offers a custom engine when no built-in default is available', () => {
    expect([
      ...getEnabledSearchEngineIds([custom], new Set(), custom.host),
    ]).toEqual(['keyword:web'])
  })

  it('preserves the legacy empty Google host ID', () => {
    const google = { ...brave, host: '' }
    expect(getSelectedSearchEngineId([google], new Set(['']), '', '')).toBe('')
  })

  it('never selects a disabled engine while another enabled engine exists', () => {
    expect(
      getSelectedSearchEngineId(
        engines,
        new Set(['keyword:web']),
        brave.host,
        brave.host,
      ),
    ).toBe('keyword:web')
  })
})
