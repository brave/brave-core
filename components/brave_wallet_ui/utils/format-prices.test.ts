import {
  formatWithCommasAndDecimals,
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from './format-prices'

describe('Check Formatting with Commas and Decimals', () => {
  it('should return an empty string if value is empty', () => {
    const value = ''
    expect(formatWithCommasAndDecimals(value)).toEqual('')
  })

  it.each([
    ['0.00', '0'],
    ['Unlimited', 'Unlimited']
  ])('should return %s if value is %s', (expected, value) => {
    expect(formatWithCommasAndDecimals(value)).toEqual(expected)
  })
})

describe('Check Formatting with Commas and Decimals for Fiat', () => {
  it('should return an empty string if value is empty', () => {
    const value = ''
    expect(formatFiatAmountWithCommasAndDecimals(value, 'USD')).toEqual('')
  })

  it.each([
    ['$0.00', '0', 'USD'],
    ['₽0.00', '0', 'RUB'],
    ['₽0.10', '0.1', 'RUB'],
    ['₽0.001', '0.001', 'RUB']
  ])('should return %s if value is %s %s', (expected, value, currency) => {
    expect(formatFiatAmountWithCommasAndDecimals(value, currency)).toEqual(expected)
  })
})

describe('Check Formatting with Commas and Decimals for Tokens', () => {
  it('should return an empty string if value is empty', () => {
    const value = ''
    const symbol = ''
    expect(formatTokenAmountWithCommasAndDecimals(value, symbol)).toEqual('')
  })

  it.each([
    ['0.00 ETH', '0', 'ETH'],
    ['0.000000000000000001 ETH', '0.000000000000000001', 'ETH']
  ])('should return %s if value is %s %s', (expected, value, symbol) => {
    expect(formatTokenAmountWithCommasAndDecimals(value, symbol)).toEqual(expected)
  })
})
