import BigNumber from 'bignumber.js'

export const formatBalance = (balance: string, decimals: number) => {
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
