/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {withUrlScheme} from './url_scheme_util'

describe('withUrlScheme', () => {
  test('should return empty for an empty url', () => {
    expect(withUrlScheme('')).toBe('')
  })

  test('should prefix a scheme-less URL with https://', () => {
    expect(withUrlScheme('example.com')).toBe('https://example.com')
  })

  test('should leave an http url untouched', () => {
    expect(withUrlScheme('http://example.com')).toBe('http://example.com')
  })

  test('should leave an https url untouched', () => {
    expect(withUrlScheme('https://example.com')).toBe('https://example.com')
  })

  test('should leave a brave url untouched', () => {
    expect(withUrlScheme('brave://settings')).toBe('brave://settings')
  })

  test('should recognize URL schemes case-insensitively', () => {
    expect(withUrlScheme('HTTPS://example.com')).toBe('HTTPS://example.com')
  })
})
