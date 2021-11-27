import {
  formatWithCommasAndDecimals,
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from './format-prices'

describe('Check Formating with Commas and Decimals', () => {
  test('Value was empty, should return an empty string', () => {
    const value = ''
    expect(formatWithCommasAndDecimals(value)).toEqual('')
  })

  test('Value is 0 should return 0.00', () => {
    const value = '0'
    expect(formatWithCommasAndDecimals(value)).toEqual('0.00')
  })
})

describe('Check Formating with Commas and Decimals for Fiat', () => {
  test('Value was empty, should return an empty string', () => {
    const value = ''
    expect(formatFiatAmountWithCommasAndDecimals(value)).toEqual('')
  })

  test('Value is 0 should return $0.00', () => {
    const value = '0'
    expect(formatFiatAmountWithCommasAndDecimals(value)).toEqual('$0.00')
  })
})

describe('Check Formating with Commas and Decimals for Tokens', () => {
  test('Value was empty, should return an empty string', () => {
    const value = ''
    const symbol = ''
    expect(formatTokenAmountWithCommasAndDecimals(value, symbol)).toEqual('')
  })

  test('Value is 0 should return 0.00 ETH', () => {
    const value = '0'
    const symbol = 'ETH'
    expect(formatTokenAmountWithCommasAndDecimals(value, symbol)).toEqual('0.00 ETH')
  })
})
