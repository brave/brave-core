import { BraveWallet } from '../constants/types'

export const stripERC20TokenImageURL = (url?: string) =>
  url?.replace('chrome://erc-token-images/', '')

export const toProperCase = (value: string) =>
  value.replace(/\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase())

export const isFromDifferentOrigin = (url?: string) =>
  url?.startsWith('http://') || url?.startsWith('https://') || url?.startsWith('data:image/') || url?.startsWith('ipfs://')

export const isValidIconExtension = (url?: string) =>
  url?.endsWith('.jpg') || url?.endsWith('.jpeg') || url?.endsWith('.png') || url?.endsWith('.svg') || url?.endsWith('.gif')

export const httpifyIpfsUrl = (url: string | undefined) => {
  if (!url) {
    return ''
  }

  return url.includes('ipfs://') ? url.replace('ipfs://', 'https://ipfs.io/ipfs/') : url
}

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
    case BraveWallet.OPTIMISM_MAINNET_CHAIN_ID:
      return 'OPTIMISM'
    case BraveWallet.FILECOIN_MAINNET:
      return 'FILECOIN'
    default:
      return ''
  }
}

export const formatAsDouble = (value: string): string =>
  // Removes all characters except numbers, commas and decimals
  value.replace(/[^0-9.,]+/g, '')
