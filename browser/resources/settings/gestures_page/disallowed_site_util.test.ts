/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  addDisallowedSiteDialogTitle, disallowedSiteRowsFromPref,
  toDisallowedSiteOrigin
} from './disallowed_site_util'

describe('toDisallowedSiteOrigin', () => {
  test('should reduce a scheme-less host to itself', () => {
    expect(toDisallowedSiteOrigin('foo.com')).toBe('foo.com')
  })

  test('should strip the scheme and path from an http URL', () => {
    expect(toDisallowedSiteOrigin('http://foo.com/bar')).toBe('foo.com')
  })

  test('should strip the scheme and path from an https URL', () => {
    expect(toDisallowedSiteOrigin('https://foo.com/bar')).toBe('foo.com')
  })

  test('should keep a non-http(s) scheme, since it names one specific page',
    () => {
      expect(toDisallowedSiteOrigin('brave://settings')).toBe('brave://settings')
    })

  test('should lowercase the result', () => {
    expect(toDisallowedSiteOrigin('HTTPS://FOO.COM')).toBe('foo.com')
  })

  test('should trim surrounding whitespace before parsing', () => {
    expect(toDisallowedSiteOrigin('  foo.com  ')).toBe('foo.com')
  })

  test('should fall back to the lowercased trimmed input when unparseable',
    () => {
      expect(toDisallowedSiteOrigin('  Not A Url  ')).toBe('not a url')
    })

  test('should return an empty string for empty input', () => {
    expect(toDisallowedSiteOrigin('')).toBe('')
  })
})

describe('disallowedSiteRowsFromPref', () => {
  test('should return an empty array when the pref is undefined', () => {
    expect(disallowedSiteRowsFromPref(undefined)).toEqual([])
  })

  test('should convert each pref dict key into a row', () => {
    expect(disallowedSiteRowsFromPref({'foo.com': true, 'bar.com': true}))
      .toEqual([{origin: 'foo.com'}, {origin: 'bar.com'}])
  })
})

describe('addDisallowedSiteDialogTitle', () => {
  const i18n = (key: string) => key

  test('should use the edit title when editing an existing site', () => {
    expect(addDisallowedSiteDialogTitle('foo.com', i18n))
      .toBe('braveGesturesEditSite')
  })

  test('should use the add title when adding a new site', () => {
    expect(addDisallowedSiteDialogTitle(null, i18n)).toBe('braveGesturesAddSite')
  })
})
