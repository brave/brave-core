import { EthereumChain } from '../constants/types'

export const GetNetworkInfo = (chainId: string, list: EthereumChain[]) => {
  for (let it of list) {
    if (it.chainId === chainId) {
      return it
    }
  }
  return { chainId: '', chainName: '', rpcUrls: [] }
}

export const reduceNetworkDisplayName = (name: string) => {
  const firstWord = name.split(' ')[0]
  if (firstWord.length > 9) {
    const firstEight = firstWord.slice(0, 6)
    const reduced = firstEight.concat('..')
    return reduced
  } else {
    return firstWord
  }
}
