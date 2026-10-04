// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  mockBraveWalletOrigin,
  mockUniswapOriginInfo,
} from '../stories/mock-data/mock-origin-info'
import {
  isRemoteImageURL,
  isValidIconExtension,
  sanitizeImageURL,
  formatAsDouble,
  formatSignMessageForDisplay,
  formatTypedDataForDisplay,
  getHiddenSignMessageCharacters,
  getTypedDataHiddenSignMessageCharacters,
  hasUnexpectedSignMessageCharacters,
  hasUnicode,
  padWithLeadingZeros,
  unicodeCharEscape,
  unicodeEscape,
  removeDoubleSpaces,
  getIsBraveWalletOrigin,
  reduceInt,
} from './string-utils'

describe('Checking URL is remote image or not', () => {
  test('HTTP URL should return true', () => {
    expect(isRemoteImageURL('http://test.com/test.png')).toEqual(true)
  })

  test('HTTPS URL should return true', () => {
    expect(isRemoteImageURL('https://test.com/test.png')).toEqual(true)
  })

  test('Data URL image should return true', () => {
    expect(
      isRemoteImageURL(
        'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAA'
          + 'CQd1PeAAAADElEQVR42mP4z8AAAAMBAQD3A0FDAAAAAElFTkSuQmCC',
      ),
    ).toEqual(true)
  })

  test('local path should return false', () => {
    expect(isRemoteImageURL('bat.png')).toEqual(false)
  })

  test('undefined input should return undefined', () => {
    expect(isRemoteImageURL(undefined)).toEqual(undefined)
  })
})

describe('sanitizeImageURL', () => {
  test('HTTPS URL should be wrapped in chrome://image proxy', () => {
    expect(sanitizeImageURL('https://example.com/logo.png')).toEqual(
      'chrome://image?url=https%3A%2F%2Fexample.com%2Flogo.png&staticEncode=true',
    )
  })

  test('HTTP URL should be wrapped in chrome://image proxy', () => {
    expect(sanitizeImageURL('http://example.com/logo.png')).toEqual(
      'chrome://image?url=http%3A%2F%2Fexample.com%2Flogo.png&staticEncode=true',
    )
  })

  test('local path should be returned unchanged', () => {
    expect(sanitizeImageURL('bat.png')).toEqual('bat.png')
  })

  test('chrome:// URL should be returned unchanged', () => {
    expect(sanitizeImageURL('chrome://erc-token-images/bat.png')).toEqual(
      'chrome://erc-token-images/bat.png',
    )
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

  it('returns "true" when a null byte is detected', () => {
    expect(hasUnicode('hello\0world')).toBe(true)
  })

  it('returns "true" when a newline byte is detected', () => {
    expect(hasUnicode('hello\nworld')).toBe(true)
  })

  it('returns "true" when a carriage return or other control is detected', () => {
    expect(hasUnicode('hello\rworld')).toBe(true)
    expect(hasUnicode('hello\tworld')).toBe(true)
    expect(hasUnicode('hello\x7fworld')).toBe(true)
  })

  it('returns "false" when Non-ASCII characters are not detected', () => {
    expect(hasUnicode('Sign into LIVE')).toBe(false)
  })
})

describe('getHiddenSignMessageCharacters', () => {
  it('names each kind of hidden character that is present', () => {
    expect(getHiddenSignMessageCharacters('plain')).toEqual([])
    expect(getHiddenSignMessageCharacters('a\0b')).toEqual(['nullByte'])
    expect(getHiddenSignMessageCharacters('a\nb')).toEqual(['newline'])
    expect(getHiddenSignMessageCharacters('a\u202Eb')).toEqual(['nonAscii'])
    expect(getHiddenSignMessageCharacters('a\rb')).toEqual(['newline'])
    expect(getHiddenSignMessageCharacters('a\x0bb')).toEqual(['newline'])
    expect(getHiddenSignMessageCharacters('a\x0cb')).toEqual(['newline'])
    expect(getHiddenSignMessageCharacters('a\tb')).toEqual(['controlCharacter'])
    expect(getHiddenSignMessageCharacters('a\x1bb')).toEqual([
      'controlCharacter',
    ])
    expect(getHiddenSignMessageCharacters('a\x7fb')).toEqual([
      'controlCharacter',
    ])
    expect(getHiddenSignMessageCharacters('a\0\nb\u202E\t')).toEqual([
      'nullByte',
      'newline',
      'nonAscii',
      'controlCharacter',
    ])
  })

  it('unions characters across every provided string', () => {
    expect(
      getHiddenSignMessageCharacters('domain\0', undefined, 'message\n\u00ff'),
    ).toEqual(['nullByte', 'newline', 'nonAscii'])
  })
})

describe('getTypedDataHiddenSignMessageCharacters', () => {
  it('detects null and newline escapes inside typed-data JSON', () => {
    expect(
      getTypedDataHiddenSignMessageCharacters(
        '{"name":"Example\\u0000 domain"}',
      ),
    ).toEqual(['nullByte'])
    expect(
      getTypedDataHiddenSignMessageCharacters('{"contents":"Hello\\nworld"}'),
    ).toEqual(['newline'])
    expect(
      getTypedDataHiddenSignMessageCharacters(
        '{"name":"Example\\u0000 domain"}',
        '{"contents":"Sign into \\u202E EVIL"}',
      ),
    ).toEqual(['nullByte', 'nonAscii'])
  })

  it('detects carriage returns and other controls inside typed-data JSON', () => {
    expect(
      getTypedDataHiddenSignMessageCharacters('{"contents":"Hello\\rworld"}'),
    ).toEqual(['newline'])
    expect(
      getTypedDataHiddenSignMessageCharacters('{"contents":"Hello\\tworld"}'),
    ).toEqual(['controlCharacter'])
  })

  it('still detects raw characters in the JSON text', () => {
    expect(
      getTypedDataHiddenSignMessageCharacters(
        '{"name":"Sign into \u202E EVIL"}',
      ),
    ).toEqual(['nonAscii'])
  })
})

describe('hasUnexpectedSignMessageCharacters', () => {
  it('is true for any hidden character and false for plain text', () => {
    expect(hasUnexpectedSignMessageCharacters(['plain'])).toBe(false)
    expect(hasUnexpectedSignMessageCharacters(['a\0b'])).toBe(true)
    expect(hasUnexpectedSignMessageCharacters(['a\nb'])).toBe(true)
    expect(hasUnexpectedSignMessageCharacters(['a\u202Eb'])).toBe(true)
    expect(hasUnexpectedSignMessageCharacters(['a\tb'])).toBe(true)
    expect(hasUnexpectedSignMessageCharacters([undefined, ''])).toBe(false)
  })

  it('decodes typed-data JSON when asked', () => {
    expect(
      hasUnexpectedSignMessageCharacters(
        ['{"name":"Example\\u0000 domain"}'],
        true,
      ),
    ).toBe(true)
    expect(
      hasUnexpectedSignMessageCharacters(['{"name":"Example domain"}'], true),
    ).toBe(false)
  })
})

describe('unicodeEscape', () => {
  it('escapes non-ASCII characters', () => {
    expect(unicodeEscape('Sign into \u202E EVIL')).toBe(
      'Sign into \\u202e EVIL',
    )
  })

  it('renders null bytes and marks newline bytes without dropping the break', () => {
    expect(unicodeEscape('hello\0world')).toBe('hello\\0world')
    expect(unicodeEscape('hello\nworld')).toBe('hello\\n\nworld')
    expect(unicodeEscape('a\0b\n\u00ff')).toBe('a\\0b\\n\n\\u00ff')
  })

  it('renders other non-printable ASCII and doubles a literal backslash', () => {
    expect(unicodeEscape('hello\rworld')).toBe('hello\\r\nworld')
    expect(unicodeEscape('hello\tworld')).toBe('hello\\u0009world')
    expect(unicodeEscape('hello\x7fworld')).toBe('hello\\u007fworld')
    expect(unicodeEscape('hello\\0world')).toBe('hello\\\\0world')
  })

  it('leaves plain text unchanged', () => {
    expect(unicodeEscape('Sign into LIVE')).toBe('Sign into LIVE')
  })
})

describe('formatSignMessageForDisplay', () => {
  it('shows the escaped message only in the formatted view', () => {
    const message = 'hello\0\n\u202E'
    expect(formatSignMessageForDisplay(message, true)).toBe(
      'hello\\0\\n\n\\u202e',
    )
    expect(formatSignMessageForDisplay(message, false)).toBe(message)
  })
})

describe('formatTypedDataForDisplay', () => {
  it('turns JSON null and newline escapes into the ASCII markers', () => {
    const domain = '{"name":"Example\\u0000 domain"}'
    const message = '{"contents":"Hello\\nworld"}'
    expect(formatTypedDataForDisplay(domain, false)).toBe(domain)
    expect(formatTypedDataForDisplay(message, false)).toBe(message)
    expect(formatTypedDataForDisplay(domain, true)).toBe(
      '{"name":"Example\\0 domain"}',
    )
    expect(formatTypedDataForDisplay(message, true)).toBe(
      '{"contents":"Hello\\n\nworld"}',
    )
  })

  it('escapes raw non-ASCII characters inside JSON strings', () => {
    expect(
      formatTypedDataForDisplay('{"name":"Sign into \u202E EVIL"}', true),
    ).toBe('{"name":"Sign into \\u202e EVIL"}')
  })

  it('falls back to a raw scan when the text is not JSON', () => {
    expect(formatTypedDataForDisplay('domain\0', true)).toBe('domain\\0')
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

describe('getIsBraveWalletOrigin', () => {
  it('should return `false` if it is not a Brave Wallet origin', () => {
    expect(getIsBraveWalletOrigin(mockUniswapOriginInfo)).toBe(false)
    expect(
      getIsBraveWalletOrigin({ originSpec: 'chrome://wallet@newtab' }),
    ).toBe(false)
  })
  it('should return `true` if it is a Brave Wallet origin', () => {
    expect(getIsBraveWalletOrigin(mockBraveWalletOrigin)).toBe(true)
  })
})

describe('reduceInt', () => {
  it('should not shorten numbers with less than 7 digits', () => {
    expect(reduceInt('1')).toBe('1')
    expect(reduceInt('123456')).toBe('123456')
  })
  it('should shorten numbers with more than 7 digits', () => {
    expect(reduceInt('12345678')).toBe('123...678')
    expect(reduceInt('1234567890')).toBe('123...890')
  })
})
