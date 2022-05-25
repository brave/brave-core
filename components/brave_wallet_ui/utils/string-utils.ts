import { BraveWallet } from '../constants/types'

export const stripERC20TokenImageURL = (url?: string) =>
  url?.replace('chrome://erc-token-images/', '')

export const toProperCase = (value: string) =>
  value.replace(/\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase())

export const isRemoteImageURL = (url?: string) =>
  url?.startsWith('http://') || url?.startsWith('https://') || url?.startsWith('data:image/')

export const isValidIconExtension = (url?: string) =>
  url?.endsWith('.jpg') || url?.endsWith('.jpeg') || url?.endsWith('.png') || url?.endsWith('.svg')

export const getRampNetworkPrefix = (chainId: string) => {
  switch (chainId) {
    case BraveWallet.MAINNET_CHAIN_ID:
    case BraveWallet.AVALANCHE_MAINNET_CHAIN_ID:
    case BraveWallet.CELO_MAINNET_CHAIN_ID:
      return ''
    case BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID:
      return 'BSC'
    case BraveWallet.POLYGON_MAINNET_CHAIN_ID:
      return 'MATIC'
    case BraveWallet.SOLANA_MAINNET:
      return 'SOLANA'
    default:
      return ''
  }
}
