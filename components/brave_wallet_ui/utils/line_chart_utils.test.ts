// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  hasDangerousKeys,
  sanitizeTimeDelta,
  sanitizeTokenPriceHistory,
  sanitizeLineChartIframeData,
} from './line_chart_utils'

describe('hasDangerousKeys', () => {
  it('should return false for a clean object', () => {
    expect(hasDangerousKeys({ foo: 'bar', baz: 123 })).toBe(false)
  })

  it('should return false for an empty object', () => {
    expect(hasDangerousKeys({})).toBe(false)
  })

  it('should return true if object has __proto__ as own property', () => {
    const obj = JSON.parse('{"__proto__": {"polluted": true}}')
    expect(hasDangerousKeys(obj)).toBe(true)
  })

  it('should return true if object has constructor as own property', () => {
    const obj = JSON.parse('{"constructor": {"prototype": {}}}')
    expect(hasDangerousKeys(obj)).toBe(true)
  })

  it('should return true if object has prototype as own property', () => {
    const obj = JSON.parse('{"prototype": {"polluted": true}}')
    expect(hasDangerousKeys(obj)).toBe(true)
  })

  it('should not flag inherited prototype properties', () => {
    // Regular objects have inherited constructor/prototype via prototype
    // chain but hasDangerousKeys checks for OWN properties only
    const obj = { normal: 'value' }
    expect(hasDangerousKeys(obj)).toBe(false)
  })
})

describe('sanitizeTimeDelta', () => {
  it('should return valid SerializableTimeDelta for valid input', () => {
    const result = sanitizeTimeDelta({ microseconds: 1234567890 })
    expect(result).toEqual({ microseconds: 1234567890 })
  })

  it('should return undefined for null', () => {
    expect(sanitizeTimeDelta(null)).toBeUndefined()
  })

  it('should return undefined for undefined', () => {
    expect(sanitizeTimeDelta(undefined)).toBeUndefined()
  })

  it('should return undefined for arrays', () => {
    expect(sanitizeTimeDelta([1, 2, 3])).toBeUndefined()
  })

  it('should return undefined for strings', () => {
    expect(sanitizeTimeDelta('1234567890')).toBeUndefined()
  })

  it('should return undefined for numbers', () => {
    expect(sanitizeTimeDelta(1234567890)).toBeUndefined()
  })

  it('should return undefined for booleans', () => {
    expect(sanitizeTimeDelta(true)).toBeUndefined()
  })

  it('should return undefined if microseconds is missing', () => {
    expect(sanitizeTimeDelta({})).toBeUndefined()
  })

  it('should return undefined if microseconds is not a number', () => {
    expect(sanitizeTimeDelta({ microseconds: '1234' })).toBeUndefined()
    expect(sanitizeTimeDelta({ microseconds: null })).toBeUndefined()
    expect(sanitizeTimeDelta({ microseconds: undefined })).toBeUndefined()
    expect(sanitizeTimeDelta({ microseconds: {} })).toBeUndefined()
  })

  it('should strip extra properties from the result', () => {
    const result = sanitizeTimeDelta({
      microseconds: 1000,
      extraProp: 'should be stripped',
      anotherProp: { nested: 'value' },
    })
    expect(result).toEqual({ microseconds: 1000 })
    expect(result).not.toHaveProperty('extraProp')
    expect(result).not.toHaveProperty('anotherProp')
  })

  it('should reject objects with __proto__ pollution', () => {
    const malicious = JSON.parse(
      '{"microseconds": 1000, "__proto__": {"polluted": true}}',
    )
    expect(sanitizeTimeDelta(malicious)).toBeUndefined()
  })

  it('should reject objects with constructor pollution', () => {
    const malicious = JSON.parse(
      '{"microseconds": 1000, "constructor": {"prototype": {}}}',
    )
    expect(sanitizeTimeDelta(malicious)).toBeUndefined()
  })

  it('should reject objects with prototype pollution', () => {
    const malicious = JSON.parse(
      '{"microseconds": 1000, "prototype": {"polluted": true}}',
    )
    expect(sanitizeTimeDelta(malicious)).toBeUndefined()
  })
})

describe('sanitizeTokenPriceHistory', () => {
  const validDate = { microseconds: 1234567890 }

  it('should return valid TokenPriceHistory for valid input', () => {
    const result = sanitizeTokenPriceHistory({
      date: validDate,
      close: 42.5,
    })
    expect(result).toEqual({
      date: { microseconds: 1234567890 },
      close: 42.5,
    })
  })

  it('should return undefined for null', () => {
    expect(sanitizeTokenPriceHistory(null)).toBeUndefined()
  })

  it('should return undefined for undefined', () => {
    expect(sanitizeTokenPriceHistory(undefined)).toBeUndefined()
  })

  it('should return undefined for arrays', () => {
    expect(sanitizeTokenPriceHistory([1, 2, 3])).toBeUndefined()
  })

  it('should return undefined for primitives', () => {
    expect(sanitizeTokenPriceHistory('string')).toBeUndefined()
    expect(sanitizeTokenPriceHistory(123)).toBeUndefined()
    expect(sanitizeTokenPriceHistory(true)).toBeUndefined()
  })

  it('should return undefined if close is missing', () => {
    expect(sanitizeTokenPriceHistory({ date: validDate })).toBeUndefined()
  })

  it('should return undefined if close is not a number', () => {
    expect(
      sanitizeTokenPriceHistory({ date: validDate, close: '42.5' }),
    ).toBeUndefined()
    expect(
      sanitizeTokenPriceHistory({ date: validDate, close: null }),
    ).toBeUndefined()
  })

  it('should return undefined if date is missing', () => {
    expect(sanitizeTokenPriceHistory({ close: 42.5 })).toBeUndefined()
  })

  it('should return undefined if date is invalid', () => {
    expect(
      sanitizeTokenPriceHistory({ date: null, close: 42.5 }),
    ).toBeUndefined()
    expect(
      sanitizeTokenPriceHistory({ date: 'invalid', close: 42.5 }),
    ).toBeUndefined()
    expect(
      sanitizeTokenPriceHistory({ date: { wrong: 'property' }, close: 42.5 }),
    ).toBeUndefined()
  })

  it('should strip extra properties from the result', () => {
    const result = sanitizeTokenPriceHistory({
      date: validDate,
      close: 42.5,
      extraProp: 'should be stripped',
      malicious: { nested: 'data' },
    })
    expect(result).toEqual({
      date: { microseconds: 1234567890 },
      close: 42.5,
    })
    expect(result).not.toHaveProperty('extraProp')
    expect(result).not.toHaveProperty('malicious')
  })

  it('should reject objects with prototype pollution', () => {
    const malicious = JSON.parse(
      '{"date": {"microseconds": 1000}, "close": 42.5, '
        + '"__proto__": {"polluted": true}}',
    )
    expect(sanitizeTokenPriceHistory(malicious)).toBeUndefined()
  })

  it('should reject if nested date has prototype pollution', () => {
    const malicious = JSON.parse(
      '{"date": {"microseconds": 1000, "__proto__": {"bad": true}}, '
        + '"close": 42.5}',
    )
    expect(sanitizeTokenPriceHistory(malicious)).toBeUndefined()
  })
})

describe('sanitizeLineChartIframeData', () => {
  const validPriceData = [
    { date: { microseconds: 1000 }, close: 100.5 },
    { date: { microseconds: 2000 }, close: 105.25 },
  ]

  const validData = {
    priceData: validPriceData,
    defaultFiatCurrency: 'USD',
    hidePortfolioBalances: false,
  }

  it('should return valid LineChartIframeData for valid input', () => {
    const result = sanitizeLineChartIframeData(validData)
    expect(result).toEqual({
      priceData: [
        { date: { microseconds: 1000 }, close: 100.5 },
        { date: { microseconds: 2000 }, close: 105.25 },
      ],
      defaultFiatCurrency: 'USD',
      hidePortfolioBalances: false,
    })
  })

  it('should allow undefined priceData', () => {
    const result = sanitizeLineChartIframeData({
      priceData: undefined,
      defaultFiatCurrency: 'EUR',
      hidePortfolioBalances: true,
    })
    expect(result).toEqual({
      priceData: undefined,
      defaultFiatCurrency: 'EUR',
      hidePortfolioBalances: true,
    })
  })

  it('should allow empty priceData array', () => {
    const result = sanitizeLineChartIframeData({
      priceData: [],
      defaultFiatCurrency: 'GBP',
      hidePortfolioBalances: false,
    })
    expect(result).toEqual({
      priceData: [],
      defaultFiatCurrency: 'GBP',
      hidePortfolioBalances: false,
    })
  })

  it('should return undefined for null', () => {
    expect(sanitizeLineChartIframeData(null)).toBeUndefined()
  })

  it('should return undefined for undefined', () => {
    expect(sanitizeLineChartIframeData(undefined)).toBeUndefined()
  })

  it('should return undefined for arrays', () => {
    expect(sanitizeLineChartIframeData([1, 2, 3])).toBeUndefined()
  })

  it('should return undefined for primitives', () => {
    expect(sanitizeLineChartIframeData('string')).toBeUndefined()
    expect(sanitizeLineChartIframeData(123)).toBeUndefined()
    expect(sanitizeLineChartIframeData(true)).toBeUndefined()
  })

  it('should return undefined if defaultFiatCurrency is missing', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
  })

  it('should return undefined if defaultFiatCurrency is not a string', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        defaultFiatCurrency: 123,
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        defaultFiatCurrency: null,
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
  })

  it('should return undefined if hidePortfolioBalances is missing', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        defaultFiatCurrency: 'USD',
      }),
    ).toBeUndefined()
  })

  it('should return undefined if hidePortfolioBalances is not boolean', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        defaultFiatCurrency: 'USD',
        hidePortfolioBalances: 'true',
      }),
    ).toBeUndefined()
    expect(
      sanitizeLineChartIframeData({
        priceData: validPriceData,
        defaultFiatCurrency: 'USD',
        hidePortfolioBalances: 1,
      }),
    ).toBeUndefined()
  })

  it('should return undefined if priceData is not an array', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: 'not an array',
        defaultFiatCurrency: 'USD',
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
    expect(
      sanitizeLineChartIframeData({
        priceData: { 0: validPriceData[0] },
        defaultFiatCurrency: 'USD',
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
  })

  it('should return undefined if any priceData item is invalid', () => {
    expect(
      sanitizeLineChartIframeData({
        priceData: [
          { date: { microseconds: 1000 }, close: 100 },
          { date: { microseconds: 2000 }, close: 'invalid' }, // invalid close
        ],
        defaultFiatCurrency: 'USD',
        hidePortfolioBalances: false,
      }),
    ).toBeUndefined()
  })

  it('should strip extra properties from the result', () => {
    const result = sanitizeLineChartIframeData({
      priceData: validPriceData,
      defaultFiatCurrency: 'USD',
      hidePortfolioBalances: true,
      extraProp: 'should be stripped',
      maliciousData: { nested: 'attack' },
    })
    expect(result).toEqual({
      priceData: [
        { date: { microseconds: 1000 }, close: 100.5 },
        { date: { microseconds: 2000 }, close: 105.25 },
      ],
      defaultFiatCurrency: 'USD',
      hidePortfolioBalances: true,
    })
    expect(result).not.toHaveProperty('extraProp')
    expect(result).not.toHaveProperty('maliciousData')
  })

  it('should reject objects with __proto__ pollution', () => {
    const malicious = JSON.parse(
      '{"priceData": [], "defaultFiatCurrency": "USD", '
        + '"hidePortfolioBalances": false, "__proto__": {"polluted": true}}',
    )
    expect(sanitizeLineChartIframeData(malicious)).toBeUndefined()
  })

  it('should reject objects with constructor pollution', () => {
    const malicious = JSON.parse(
      '{"priceData": [], "defaultFiatCurrency": "USD", '
        + '"hidePortfolioBalances": false, "constructor": {"prototype": {}}}',
    )
    expect(sanitizeLineChartIframeData(malicious)).toBeUndefined()
  })

  it('should reject if priceData items have prototype pollution', () => {
    const malicious = JSON.parse(
      '{"priceData": [{"date": {"microseconds": 1000}, "close": 100, '
        + '"__proto__": {"bad": true}}], "defaultFiatCurrency": "USD", '
        + '"hidePortfolioBalances": false}',
    )
    expect(sanitizeLineChartIframeData(malicious)).toBeUndefined()
  })

  it('should reject deeply nested prototype pollution in dates', () => {
    const malicious = JSON.parse(
      '{"priceData": [{"date": {"microseconds": 1000, '
        + '"__proto__": {"bad": true}}, "close": 100}], '
        + '"defaultFiatCurrency": "USD", "hidePortfolioBalances": false}',
    )
    expect(sanitizeLineChartIframeData(malicious)).toBeUndefined()
  })
})

describe('integration: JSON parsing from external sources', () => {
  // Note: decodeURIComponent is mocked globally in testPolyfills.ts,
  // so we test JSON parsing directly without URL encoding/decoding

  it('should safely parse and sanitize valid JSON', () => {
    const jsonString =
      '{"priceData":[{"date":{"microseconds":1000},"close":50}],'
      + '"defaultFiatCurrency":"USD","hidePortfolioBalances":false}'
    const parsed = JSON.parse(jsonString)
    const result = sanitizeLineChartIframeData(parsed)

    expect(result).toEqual({
      priceData: [{ date: { microseconds: 1000 }, close: 50 }],
      defaultFiatCurrency: 'USD',
      hidePortfolioBalances: false,
    })
  })

  it('should reject malicious JSON with prototype pollution', () => {
    const maliciousJson =
      '{"__proto__": {"isAdmin": true}, '
      + '"defaultFiatCurrency": "USD", "hidePortfolioBalances": false}'
    const parsed = JSON.parse(maliciousJson)
    const result = sanitizeLineChartIframeData(parsed)

    expect(result).toBeUndefined()
  })

  it('should handle malformed JSON gracefully', () => {
    const malformedJson = '{invalid json}'
    expect(() => JSON.parse(malformedJson)).toThrow()
    // The actual component wraps this in try-catch and returns undefined
  })
})
