// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { extractQuery, matches } from './query'

describe('query', () => {
  describe('matches', () => {
    it('should return true if the query is a substring of the text', () => {
      expect(matches('hello', 'hello world')).toBe(0)
    })

    it('should return false if the query is not a substring of the text', () => {
      expect(matches('bye', 'hello world')).toBe(-1)
    })

    it('should return true if the query substring of the text case insensitive', () => {
      expect(matches('HELLO', 'HeLlO world')).toBe(0)
    })

    it('should match ignoring whitespace', () => {
      expect(matches('HELLO', 'He LlO   world')).toBe(0)
    })

    it('empty string should match anything', () => {
      expect(matches('', 'He LlO   world')).toBe(0)
    })

    it('empty strings should match', () => {
      expect(matches('', '')).toBe(0)
    })
  })

  describe('extractQuery', () => {
    it('should return the query if it is at the start of the text', () => {
      expect(
        extractQuery('/hello world', {
          onlyAtStart: true,
          triggerCharacter: '/',
        }),
      ).toBe('hello world')
    })

    it('should not return the query if it is not at the start of the text', () => {
      expect(
        extractQuery('hi /hello world', {
          onlyAtStart: true,
          triggerCharacter: '/',
        }),
      ).toBeNull()
    })

    it('should return the empty string if the trigger character is in the string but no query', () => {
      expect(
        extractQuery('/', { onlyAtStart: true, triggerCharacter: '/' }),
      ).toBe('')
    })

    it('empty string should return null', () => {
      expect(
        extractQuery('', { onlyAtStart: true, triggerCharacter: '/' }),
      ).toBeNull()
    })

    it('should support @ as trigger character', () => {
      expect(
        extractQuery('@hello world', {
          onlyAtStart: true,
          triggerCharacter: '@',
        }),
      ).toBe('hello world')
    })

    it('should support matches not just at the start', () => {
      expect(
        extractQuery('hi @hello world', {
          onlyAtStart: false,
          triggerCharacter: '@',
        }),
      ).toBe('hello world')
    })
  })
})
