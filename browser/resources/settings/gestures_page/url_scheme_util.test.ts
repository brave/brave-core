/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {withUrlScheme, toUrl} from './url_scheme_util'

describe('withUrlScheme', () => {
  test('should return an empty string for an empty URL', () => {
    expect(withUrlScheme('')).toBe('')
  })

  test('should prefix a scheme-less URL with https://', () => {
    expect(withUrlScheme('foo.com')).toBe('https://foo.com')
  })

  test('should preserve HTTP URLs', () => {
    expect(withUrlScheme('http://foo.com')).toBe('http://foo.com')
  })

  test('should preserve HTTPS URLs', () => {
    expect(withUrlScheme('https://foo.com')).toBe('https://foo.com')
  })

  test('should preserve Brave scheme URLs', () => {
    expect(withUrlScheme('brave://settings')).toBe('brave://settings')
  })

  test('should recognize URL schemes case-insensitively', () => {
    expect(withUrlScheme('HTTPS://foo.com')).toBe('HTTPS://foo.com')
  })
})

describe('toUrl', () => {
  test('should parse a scheme-less host by retrying with https:// prepended', () => {
    expect(toUrl('foo.com')?.href).toBe('https://foo.com/')
  })

  test('should parse a URL that already has a scheme as-is', () => {
    expect(toUrl('brave://settings')?.href).toBe('brave://settings/')
  })

  test('should return null for input that cannot be parsed as a URL', () => {
    expect(toUrl('not a url')).toBeNull()
  })

  test('should return null for empty input', () => {
    expect(toUrl('')).toBeNull()
  })
})
