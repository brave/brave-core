import { BraveWallet } from '../constants/types'
export const emptyNetwork = {
  chainId: '',
  chainName: '',
  rpcUrls: [],
  blockExplorerUrls: [],
  iconUrls: [],
  symbol: '',
  symbolName: '',
  decimals: 0,
  coin: BraveWallet.CoinType.ETH,
  data: {
     ethData: {
      isEip1559: true
     }
  }
}

export const GetNetworkInfo = (chainId: string, list: BraveWallet.NetworkInfo[]) => {
  for (let it of list) {
    if (it.chainId === chainId) {
      return it
    }
  }
  return emptyNetwork
}

export const reduceNetworkDisplayName = (name: string) => {
  if (!name) {
    return ''
  } else {
    const firstWord = name.split(' ')[0]
    if (firstWord.length > 9) {
      const firstEight = firstWord.slice(0, 6)
      const reduced = firstEight.concat('..')
      return reduced
    } else {
      return firstWord
    }
  }
}
