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

export const getNetworkInfo = (chainId: string, list: BraveWallet.NetworkInfo[]) => {
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

export const getNetworksByCoinType = (networks: BraveWallet.NetworkInfo[], coin: BraveWallet.CoinType): BraveWallet.NetworkInfo[] => {
  if (!networks) {
    return []
  }
  return networks.filter((network) => network.coin === coin)
}

export const getTokensNetwork = (networks: BraveWallet.NetworkInfo[], token: BraveWallet.BlockchainToken): BraveWallet.NetworkInfo => {
  if (!networks) {
    return emptyNetwork
  }

  const network = networks.filter((n) => n.chainId === token.chainId)
  if (network.length > 1) {
    return network?.find((n) => n.symbol.toLowerCase() === token.symbol.toLowerCase()) ?? emptyNetwork
  }

  return network[0] ?? emptyNetwork
}

export const getTokensCoinType = (networks: BraveWallet.NetworkInfo[], token: BraveWallet.BlockchainToken) => {
  if (!networks) {
    return ''
  }
  return getTokensNetwork(networks, token).coin || ''
}

export const getCoinFromTxDataUnion = (txDataUnion: BraveWallet.TxDataUnion): BraveWallet.CoinType => {
  if (txDataUnion.filTxData) { return BraveWallet.CoinType.FIL }
  if (txDataUnion.solanaTxData) { return BraveWallet.CoinType.SOL }
  return BraveWallet.CoinType.ETH
}

export const getNetworkFromTXDataUnion = (txDataUnion: BraveWallet.TxDataUnion, networks: BraveWallet.NetworkInfo[], selectedNetwork: BraveWallet.NetworkInfo) => {
  const coin = getCoinFromTxDataUnion(txDataUnion)
  return networks.find((network) => network.coin === coin) ?? selectedNetwork
}
