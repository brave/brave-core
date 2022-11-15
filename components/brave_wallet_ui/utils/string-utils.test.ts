// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  isRemoteImageURL,
  isValidIconExtension,
  formatAsDouble,
  hasUnicode,
  padWithLeadingZeros,
  unicodeCharEscape,
  removeDoubleSpaces
} from './string-utils'

describe('Checking URL is remote image or not', () => {
  test('HTTP URL should return true', () => {
    expect(isRemoteImageURL('http://test.com/test.png')).toEqual(true)
  })

  test('HTTPS URL should return true', () => {
    expect(isRemoteImageURL('https://test.com/test.png')).toEqual(true)
  })

  test('Data URL image should return true', () => {
    expect(isRemoteImageURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR42mP4z8AAAAMBAQD3A0FDAAAAAElFTkSuQmCC')).toEqual(true)
  })

  test('local path should return false', () => {
    expect(isRemoteImageURL('bat.png')).toEqual(false)
  })

  test('undefined input should return undefined', () => {
    expect(isRemoteImageURL(undefined)).toEqual(undefined)
  })
})

describe('Checking URL ends with a valid icon extension', () => {
  test('Ends with .png should return true', () => {
    expect(isValidIconExtension('http://test.com/test.png')).toEqual(true)
  })

  test('Ends with .svg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.svg')).toEqual(true)
  })

  test('Ends with .jpg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.jpg')).toEqual(true)
  })

  test('Ends with .jpeg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.jpeg')).toEqual(true)
  })

  test('Ends with .com should return false', () => {
    expect(isValidIconExtension('https://test.com/')).toEqual(false)
  })
})

describe('Check toDouble values', () => {
  test('Value with a USD symbol, should remove the USD symbol', () => {
    expect(formatAsDouble('$1,689.16')).toEqual('1,689.16')
  })
  test('Value with a Euro symbol, should remove the Euro symbol', () => {
    expect(formatAsDouble('689,16€')).toEqual('689,16')
  })
})

describe('hasUnicode', () => {
  it('returns "true" when Non-ASCII characters are detected', () => {
    expect(hasUnicode('Sign into \u202E EVIL')).toBe(true)
  })

  it('returns "false" when Non-ASCII characters are not detected', () => {
    expect(hasUnicode('Sign into LIVE')).toBe(false)
  })
})

describe('padWithLeadingZeros', () => {
  it('should add three zeros to the beginning of a single character string', () => {
    expect(padWithLeadingZeros('Z')).toBe('000Z')
  })
  it('should add two zeros to the beginning of a 2-character string', () => {
    expect(padWithLeadingZeros('AB')).toBe('00AB')
  })
  it('should add one zero to the beginning of a 3-character string', () => {
    expect(padWithLeadingZeros('ABC')).toBe('0ABC')
  })
  it('should not add zeros to the beginning of a 4+ character string', () => {
    expect(padWithLeadingZeros('ABCD')).toBe('ABCD')
  })
})

describe('unicodeCharEscape', () => {
  it('should return the escaped unicode value string of a unicode character', () => {
    expect(unicodeCharEscape(300)).toBe('\\u012c')
    expect(unicodeCharEscape(550)).toBe('\\u0226')
    expect(unicodeCharEscape(1550)).toBe('\\u060e')
  })
})

describe('removeDoubleSpaces', () => {
  it('should remove all instances of multiple spaces " " from a string', () => {
    expect(removeDoubleSpaces('word  word word')).toBe('word word word')
    expect(removeDoubleSpaces('word  word  word')).toBe('word word word')
    expect(removeDoubleSpaces('word  word')).toBe('word word')
    expect(removeDoubleSpaces('word')).toBe('word')
  })
})
