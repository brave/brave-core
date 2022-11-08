// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import Amount from './amount'

let languageGetter

beforeEach(() => {
  languageGetter = jest.spyOn(window.navigator, 'language', 'get')
  languageGetter.mockReturnValue('en-GB')
})

describe('Amount class', () => {
  describe('instantiation tests', () => {
    it('should create Amount object using empty value', () => {
      expect(Amount.empty().value).toBeUndefined()
      expect(Amount.empty().isUndefined()).toBeTruthy()
      expect(Amount.empty().format()).toBe('')
    })

    it('should create Amount object using non-empty value', () => {
      expect(new Amount('123').value).not.toBeUndefined()
      expect(new Amount('123').format()).not.toBe('')
    })

    it('should return zero-value amount with .zero()', () => {
      expect(Amount.zero().format()).toBe('0')
      expect(Amount.zero().isZero()).toBeTruthy()
    })
  })

  describe('precision tests', () => {
    it.each([
      ['0', 6, '0'], // zero values are always formatted to 0
      ['100', 6, '100'], // integer values are not formatted with unnecessary precision
      ['100001.1', 6, '100001.1'],
      ['10001.1', 6, '10001.1'],
      ['0.0000000000001', 6, '0.0000000000001'],
      ['123456789.0', 6, '123456789'],
      ['12345678.90', 6, '12345678.9'],
      ['1234567.890', 6, '1234567.89'],
      ['123456.7890', 6, '123456.79'],
      ['12345.67890', 6, '12345.68']
    ])('should format asset amount %s to %d significant digits', (value: string, sd: number, expected: string) => {
      expect(new Amount(value).format(sd)).toBe(expected)
    })

    it('should optionally format asset amount 100001.1 with full precision', () => {
      expect(new Amount('100001.1').format()).toBe('100001.1')
    })

    it.each([
      ['0', '0.00'],
      ['100', '100.00'],
      ['100001.1', '100,001.10'],
      ['10001.1', '10,001.10'],
      ['0.0000000000001', '0.0000000000001'],
      ['123456789.0', '123,456,789.00'],
      ['12345678.90', '12,345,678.90'],
      ['1234567.890', '1,234,567.89'],
      ['123456.7890', '123,456.79'],
      ['12345.67890', '12,345.68']
    ])('should format fiat amount %s to %s', (value: string, expected: string) => {
      expect(new Amount(value).formatAsFiat()).toBe(expected)
    })
  })

  describe('decimals conversion tests', () => {
    it('should convert from Wei to ETH', () => {
      expect(new Amount('1').divideByDecimals(18).format()).toBe('0.000000000000000001')
    })

    it('should convert from ETH to Wei', () => {
      expect(new Amount('1').multiplyByDecimals(18).format()).toBe('1000000000000000000')
    })

    it('should convert from Wei to GWei', () => {
      expect(new Amount('10000000000').divideByDecimals(9).format()).toBe('10')
    })

    it('should handle decimal conversion on undefined amounts', () => {
      expect(Amount.empty().divideByDecimals(18).isUndefined()).toBeTruthy()
      expect(Amount.empty().multiplyByDecimals(18).isUndefined()).toBeTruthy()
    })
  })

  describe('normalization tests', () => {
    it('should normalize a hex numeric value', () => {
      expect(Amount.normalize('0x2')).toEqual('2')
      expect(Amount.normalize('0x0')).toEqual('0')
    })

    it('should normalize invalid numeric values to empty string', () => {
      expect(Amount.normalize('')).toEqual('')
      expect(Amount.normalize('0x')).toEqual('')
      expect(Amount.normalize('invalid value')).toEqual('')
    })
  })

  describe('numeric operations tests', () => {
    it('should multiply two amounts', () => {
      // OK: 10 x undefined = undefined
      expect(new Amount('10').times('').format()).toBe('')

      // OK: undefined x 10 = undefined
      expect(Amount.empty().times('10').format()).toBe('')

      // Multiply two integer amounts
      expect(new Amount('10').times(10).format()).toBe('100')

      // Multiply an integer amount with a float
      expect(new Amount('100').times(0.01).format()).toBe('1')

      // Multiply an integer amount with a wrapped amount object
      expect(new Amount('100').times(new Amount('0.01')).format()).toBe('1')
      expect(new Amount('100').times(Amount.empty()).format()).toBe('')
    })

    it('should add two amounts', () => {
      // OK: 10 + undefined = 10
      expect(new Amount('10').plus('').format()).toBe('10')

      // OK: undefined + 10 = 10
      expect(Amount.empty().plus('10').format()).toBe('10')

      // Add two float amounts
      expect(new Amount(10.1).plus('9.9').format()).toBe('20')

      // Add a float amount with an integer
      expect(new Amount(10.1).plus('9').format()).toBe('19.1')

      // Add an integer amount with a wrapped amount object
      expect(new Amount(10.1).plus(new Amount('9')).format()).toBe('19.1')
      expect(new Amount(10.1).plus(Amount.empty()).format()).toBe('10.1')
    })

    it('should divide two amounts', () => {
      // OK: 10 / undefined = undefined
      expect(new Amount('10').div('').format()).toBe('')
      expect(new Amount('10').div('0').format()).toBe('')
      expect(new Amount('10').div('0.0').format()).toBe('')

      // OK: undefined / 10 = undefined
      expect(Amount.empty().div('10').format()).toBe('')

      // Divide two integer amounts resulting in an integer amount
      expect(new Amount('10').div('5').format()).toBe('2')

      // Divide two integer amounts resulting in a float amount
      expect(new Amount('10').div('3').format(6)).toBe('3.33333')

      // Divide an integer amount by a float amount
      expect(new Amount('10').div('3.5').format(6)).toBe('2.85714')

      // Divide an integer amount by a wrapped amount object
      expect(new Amount('10').div(new Amount('3.5')).format(6)).toBe('2.85714')
      expect(new Amount('10').div(Amount.empty()).format(6)).toBe('')
    })
  })

  describe('comparison operations tests', () => {
    it('should return correct comparison for .gt()', () => {
      // Compare two hex amounts
      // OK: 0x2 > 0x3 == false
      expect(new Amount('0x2').gt('0x3')).toBeFalsy()

      // Compare two integer amounts
      // OK: 2 > 3 == false
      expect(new Amount(2).gt(3)).toBeFalsy()

      // Compare two integer amounts
      // OK: 3 > 2 == true
      expect(new Amount(3).gt(2)).toBeTruthy()

      // Compare two hex amounts
      // OK: 0x3 > 0x2 == true
      expect(new Amount('0x3').gt('0x2')).toBeTruthy()

      // Compare with a wrapped Amount object
      // OK: 0x3 > Amount(0x2) == true
      expect(new Amount('0x3').gt(new Amount('0x2'))).toBeTruthy()

      // Compare with undefined amount
      // OK: undefined > 3 == false
      expect(Amount.empty().gt('3')).toBeFalsy()

      // Compare with undefined amount
      // OK: 3 > undefined == false
      expect(new Amount('3').gt(Amount.empty())).toBeFalsy()
    })

    it('should return correct comparison for .gte()', () => {
      expect(new Amount(2).gte(3)).toBeFalsy()
      expect(new Amount(2).gte(2)).toBeTruthy()
    })

    it('should return correct comparison for .lt()', () => {
      // Compare two hex amounts
      // OK: 0x3 < 0x2 == false
      expect(new Amount('0x3').lt('0x2')).toBeFalsy()

      // Compare two integer amounts
      // OK: 3 < 2 == false
      expect(new Amount(3).lt(2)).toBeFalsy()

      // Compare two integer amounts
      // OK: 2 < 3 == true
      expect(new Amount(2).lt(3)).toBeTruthy()

      // Compare two hex amounts
      // OK: 0x2 < 0x3 == true
      expect(new Amount('0x2').lt('0x3')).toBeTruthy()

      // Compare with a wrapped Amount object
      // OK: 0x2 < Amount(0x3) == true
      expect(new Amount('0x2').lt(new Amount('0x3'))).toBeTruthy()

      // Compare with undefined amount
      // OK: undefined < 3 == false
      expect(Amount.empty().lt('3')).toBeFalsy()

      // Compare with undefined amount
      // OK: 3 < undefined == false
      expect(new Amount('3').lt(Amount.empty())).toBeFalsy()
    })

    it('should return correct comparison for .lte()', () => {
      expect(new Amount(3).lte(2)).toBeFalsy()
      expect(new Amount(2).lte(2)).toBeTruthy()
    })

    it('should return correct comparison for .eq()', () => {
      expect(Amount.empty().eq('')).toBeTruthy()
      expect(new Amount('123.999').eq('123.999')).toBeTruthy()
      expect(new Amount('123.999').eq(123.999)).toBeTruthy()
      expect(new Amount('0x2').eq(2)).toBeTruthy()
      expect(new Amount('0.00').eq(0)).toBeTruthy()
      expect(new Amount('123.99').eq(new Amount('123.99'))).toBeTruthy()
      expect(new Amount('123').eq('456')).toBeFalsy()
      expect(new Amount('123').eq(Amount.empty())).toBeFalsy()
    })
  })

  describe('convenience methods', () => {
    it('should return correct result for .isUndefined()', () => {
      expect(Amount.empty().isUndefined()).toBeTruthy()
      expect(new Amount('123').isUndefined()).toBeFalsy()
    })

    it('should return correct result for .isNaN()', () => {
      expect(new Amount('invalid value').isNaN()).toBeTruthy()
      expect(new Amount('123').isNaN()).toBeFalsy()
      expect(Amount.empty().isNaN()).toBeFalsy()
    })

    it('should return the correct result for .isZero()', () => {
      expect(new Amount('0').isZero()).toBeTruthy()
      expect(new Amount('0.1').isZero()).toBeFalsy()
    })

    it('should return the correct result for .isPositive()', () => {
      expect(new Amount('123').isPositive()).toBeTruthy()
      expect(new Amount('0').isPositive()).toBeTruthy()
      expect(new Amount('-1').isPositive()).toBeFalsy()
    })

    it('should return the correct result for .isNegative()', () => {
      expect(new Amount('-123').isNegative()).toBeTruthy()
      expect(new Amount('0').isNegative()).toBeFalsy()
      expect(new Amount('0.1').isNegative()).toBeFalsy()
    })

    it('should return the correct result for .toHex()', () => {
      expect(new Amount('12345').toHex()).toBe('0x3039')
      expect(new Amount('0').toHex()).toBe('0x0')
      expect(Amount.empty().toHex()).toBe('')
      expect(new Amount('invalid value').toHex()).toBe('0x0')
    })

    it('should return the correct result for .toNumber()', () => {
      expect(new Amount('123').toNumber()).toBe(123)
      expect(Amount.empty().toNumber()).toBe(0)
      expect(new Amount('invalid value').toNumber()).toBe(0)
    })
  })

  describe('formatting tests', () => {
    it('should return an empty string if amount is undefined', () => {
      expect(Amount.empty().format()).toBe('')

      expect(Amount.empty().formatAsAsset()).toBe('')
      expect(Amount.empty().formatAsAsset(6)).toBe('')
      expect(Amount.empty().formatAsAsset(undefined, 'XYZ')).toBe('')
      expect(Amount.empty().formatAsAsset(6, 'XYZ')).toBe('')

      expect(Amount.empty().formatAsFiat()).toBe('')
      expect(Amount.empty().formatAsFiat('USD')).toBe('')

      expect(new Amount('invalid value').format()).toBe('')
      expect(new Amount('invalid value').formatAsAsset()).toBe('')
      expect(new Amount('invalid value').formatAsFiat()).toBe('')
    })

    it('should format 0 amount correctly', () => {
      expect(Amount.zero().format()).toBe('0')
      expect(Amount.zero().formatAsAsset()).toBe('0')
      expect(Amount.zero().formatAsFiat()).toBe('0.00')
    })

    it('should format amount with commas', () => {
      expect(new Amount('1.23456789').format(undefined, true)).toBe('1.23456789')
      expect(new Amount('12345.6789').format(undefined, true)).toBe('12,345.6789')
      expect(new Amount('12345678.9').format(undefined, true)).toBe('12,345,678.9')

      expect(new Amount('1.23456789').formatAsFiat()).toBe('1.235')
      expect(new Amount('12345.6789').formatAsFiat()).toBe('12,345.68')
      expect(new Amount('12345678.9').formatAsFiat()).toBe('12,345,678.90')
    })

    it('should format amount with symbol', () => {
      expect(new Amount('123.456').formatAsAsset(undefined, 'ETH')).toBe('123.456 ETH')
    })

    it.each([
      ['0', 'USD', '$0.00'],
      ['0', 'RUB', '₽0.00'],
      ['0.1', 'RUB', '₽0.10'],
      ['0.001', 'RUB', '₽0.001']
    ])('should format fiat amount %s with currency %s as %s', (value, currency, expected) => {
      expect(new Amount(value).formatAsFiat(currency)).toBe(expected)
    })

    it.each([
      ['122', 2, 'USD', '$122.00'],
      ['567', 2, undefined, '567.00'],
      ['1234', 2, 'USD', '$1.23k'],
      ['1234', 2, undefined, '1.23k'],
      ['1361000', 3, 'USD', '$1.361M'],
      ['1361000', 2, undefined, '1.36M'],
      ['1361500000', 3, 'USD', '$1.362B'],
      ['1361500000', 2, undefined, '1.36B'],
      ['1358900000000', 3, 'USD', '$1.359T'],
      ['1358900000000000', 3, 'USD', '$1,358.900T']
    ])('should abbreviate amount %s to have %s decimal places with currency %s as %s', (value: string, decimals: number, currency: string, expected: string) => {
      expect(new Amount(value).abbreviate(decimals, currency)).toBe(expected)
    })
  })
})
