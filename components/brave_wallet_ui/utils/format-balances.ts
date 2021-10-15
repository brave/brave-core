import BigNumber from 'bignumber.js'

export const formatBalance = (balance: string, decimals: number) => {
  if (decimals == 0) {
    const result = new BigNumber(balance)
    return (result.isNaN()) ? '0' : result.toFixed(0)
  }

  const result = new BigNumber(balance).dividedBy(10 ** decimals)
  return (result.isNaN()) ? '0.0000' : result.toFixed(4, BigNumber.ROUND_UP)
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
  const formattedBalance = formatBalance(balance, decimals)
  const result = new BigNumber(formattedBalance).multipliedBy(price)
  return (result.isNaN()) ? '0.00' : result.toFixed(2, BigNumber.ROUND_UP)
}

export const toWei = (value: string, decimals: number) => {
  const result = new BigNumber(value).multipliedBy(10 ** decimals)
  return result.isNaN() ? '0' : result.toFixed(0)
}

export const toWeiHex = (value: string, decimals: number) => {
  const result = new BigNumber(value).multipliedBy(10 ** decimals)
  return (result.isNaN()) ? '0x0' : '0x' + result.toString(16)
}

export const toGWei = (value: string, decimals: number) => {
  const result = new BigNumber(value).dividedBy(10 ** decimals).multipliedBy(10 ** 9)
  return result.isNaN() ? '0' : result.toFixed(2).replace(/\.00$/,'')
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
