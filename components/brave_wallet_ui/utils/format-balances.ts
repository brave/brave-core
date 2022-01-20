import BigNumber from 'bignumber.js'

export const formatInputValue = (value: string, decimals: number, round = true) => {
  if (decimals === 0) {
    const result = new BigNumber(value)
    return (result.isNaN()) ? '0' : result.toFixed(0)
  }

  const result = new BigNumber(value).dividedBy(10 ** decimals)
  if (result.isNaN()) {
    return '0'
  }

  // We format the number to have 6 places of decimals, unless the value
  // is too small.
  //
  // For situations where the value must not be rounded, round flag may be
  // used.
  const formattedValue = (result.isGreaterThanOrEqualTo(0.000001) && result.decimalPlaces() > 6 && round)
    ? result.toFixed(6)
    : result.toFixed()

  // Remove trailing zeros, including the decimal separator if necessary.
  // Example: 1.0000000000 becomes 1.
  return formattedValue.replace(/\.0*$|(\.\d*[1-9])0+$/, '$1')
}

export const formatBalance = (balance: string, decimals: number, round: boolean = true) => {
  if (!balance) {
    return ''
  }

  if (decimals === 0) {
    const result = new BigNumber(balance)
    return (result.isNaN()) ? '0' : result.toFixed(0)
  }

  const result = new BigNumber(balance).dividedBy(10 ** decimals)
  if (result.isNaN()) {
    return '0.0000'
  }

  return round
    ? result.toFixed(4, BigNumber.ROUND_UP)
    : result.toFormat()
}

export const formatGasFee = (gasPrice: string, gasLimit: string, decimals: number) => {
  const result = new BigNumber(gasPrice).multipliedBy(new BigNumber(gasLimit)).dividedBy(10 ** decimals)
  return (result.isNaN()) ? '0.000000' : result.toFixed(6, BigNumber.ROUND_UP)
}

export const formatFiatGasFee = (gasFee: string, price: string) => {
  const result = new BigNumber(gasFee).multipliedBy(price)
  return (result.isNaN()) ? '0.00' : result.toFixed(2, BigNumber.ROUND_UP)
}

export const formatGasFeeFromFiat = (gasFee: string, price: string) => {
  const result = new BigNumber(gasFee).dividedBy(price)
  return (result.isNaN()) ? '0.00' : result.toFixed(2, BigNumber.ROUND_UP)
}

export const formatFiatBalance = (balance: string, decimals: number, price: string) => {
  if (!price) {
    return ''
  }

  const formattedBalance = formatBalance(balance, decimals)
  const result = new BigNumber(formattedBalance).multipliedBy(price)
  return (result.isNaN()) ? '0.00' : result.toFixed(2, BigNumber.ROUND_UP)
}

export const toWei = (value: string, decimals: number) => {
  const result = new BigNumber(value).multipliedBy(10 ** decimals)
  return result.isNaN() ? '0' : result.toFixed(0)
}

export const toHex = (value: string) => {
  const numberValue = Number(value)
  return '0x' + numberValue.toString(16)
}

export const hexToNumber = (value: string, hideSymbol?: boolean) => {
  if (value) {
    return hideSymbol ? parseInt(value, 16) : '#' + parseInt(value, 16)
  }
  return ''
}

export const toWeiHex = (value: string, decimals: number) => {
  const result = new BigNumber(value).multipliedBy(10 ** decimals)
  return (result.isNaN()) ? '0x0' : '0x' + result.toString(16)
}

export const toGWei = (value: string) => {
  const result = new BigNumber(value).dividedBy(10 ** 9)
  return result.isNaN() ? '0' : result.toFixed(2).replace(/\.00$/, '')
}

export const gWeiToWei = (value: string) => {
  const result = new BigNumber(value).multipliedBy(10 ** 9)
  return result.isNaN() ? '0' : result.toFixed(0)
}

export const gWeiToWeiHex = (value: string) => {
  const result = new BigNumber(value).multipliedBy(10 ** 9)
  return result.isNaN() ? '0x0' : '0x' + result.toString(16)
}

export const addCurrencies = (first: string, second: string) => {
  const result = new BigNumber(first).plus(new BigNumber(second))
  return `0x${result.toString(16)}`
}

export const formatHexStrToNumber = (value: string): string => {
  const result = new BigNumber(value)
  return result.isNaN() ? '0' : result.toFixed(0)
}
