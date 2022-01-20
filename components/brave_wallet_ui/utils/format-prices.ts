import BigNumber from 'bignumber.js'

import { CurrencySymbols } from './currency-symbols'

export const formatWithCommasAndDecimals = (value: string, decimals?: number) => {
  // empty string indicates unknown balance
  if (value === '') {
    return ''
  }

  const valueBN = new BigNumber(value)

  // We sometimes return Unlimited as a value
  if (valueBN.isNaN()) {
    return value
  }

  if (valueBN.isZero()) {
    return '0.00'
  }

  const fmt = {
    decimalSeparator: '.',
    groupSeparator: ',',
    groupSize: 3
  } as BigNumber.Format

  if (decimals && valueBN.isGreaterThan(10 ** -decimals)) {
    return valueBN.toFormat(decimals, BigNumber.ROUND_UP, fmt)
  }

  if (valueBN.isGreaterThanOrEqualTo(10)) {
    return valueBN.toFormat(2, BigNumber.ROUND_UP, fmt)
  }

  if (valueBN.isGreaterThanOrEqualTo(1)) {
    return valueBN.toFormat(3, BigNumber.ROUND_UP, fmt)
  }

  return valueBN.toFormat(fmt)
}

export const formatFiatAmountWithCommasAndDecimals = (value: string, defaultCurrency: string): string => {
  if (!value) {
    return ''
  }

  const currencySymbol = CurrencySymbols[defaultCurrency]

  // Check to make sure a formatted value is returned before showing the fiat symbol
  if (!formatWithCommasAndDecimals(value, 2)) {
    return ''
  }
  return currencySymbol + formatWithCommasAndDecimals(value, 2)
}

export const formatTokenAmountWithCommasAndDecimals = (value: string, symbol: string): string => {
  // Empty string indicates unknown balance
  if (!value && !symbol) {
    return ''
  }
  // Check to make sure a formatted value is returned before showing the symbol
  if (!formatWithCommasAndDecimals(value)) {
    return ''
  }
  return formatWithCommasAndDecimals(value) + ' ' + symbol
}
