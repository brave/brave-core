// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, WalletRoutes, TokenStandards } from '../constants/types'
import { getLocale } from '../../common/locale'
import { loadTimeData } from '../../common/loadTimeData'

export const stripChromeImageURL = (url?: string) =>
  url?.replace('chrome://image?', '')

export const stripERC20TokenImageURL = <T extends string | undefined>(
  url?: T,
): T => {
  if (url) {
    return url.replace('chrome://erc-token-images/', '') as T
  }
  return undefined as T
}

export const toProperCase = (value: string) =>
  value.replace(
    /\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase(),
  )

export function getIsBraveWalletOrigin({
  originSpec,
}: Pick<BraveWallet.OriginInfo, 'originSpec'>) {
  try {
    const url = new URL(originSpec)
    return (
      (url.protocol === 'chrome:' || url.protocol === 'brave:')
      && url.host === 'wallet'
    )
  } catch (error) {
    console.log(error)
    return false
  }
}

export const isRemoteImageURL = (url?: string) =>
  url?.startsWith('http://')
  || url?.startsWith('https://')
  || url?.startsWith('data:image/')
  || isIpfs(url)

export const isValidIconExtension = (url?: string) =>
  url?.endsWith('.jpg')
  || url?.endsWith('.jpeg')
  || url?.endsWith('.png')
  || url?.endsWith('.svg')
  || url?.endsWith('.gif')

export const isDataURL = (url?: string) =>
  url?.startsWith('chrome://erc-token-images/')

export const getRampNetworkPrefix = (chainId: string, isOfframp?: boolean) => {
  switch (chainId) {
    // Offramp uses ETH prefix
    case BraveWallet.MAINNET_CHAIN_ID:
      return isOfframp ? 'ETH' : ''
    case BraveWallet.AVALANCHE_MAINNET_CHAIN_ID:
      return 'AVAXC'
    case BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID:
      return 'BSC'
    case BraveWallet.POLYGON_MAINNET_CHAIN_ID:
      return 'MATIC'
    case BraveWallet.SOLANA_MAINNET:
      return 'SOLANA'
    case BraveWallet.OPTIMISM_MAINNET_CHAIN_ID:
      return 'OPTIMISM'
    // Offramp uses CELO prefix
    case BraveWallet.CELO_MAINNET_CHAIN_ID:
      return isOfframp ? 'CELO' : ''
    case BraveWallet.FANTOM_MAINNET_CHAIN_ID:
      return 'FANTOM'
    case BraveWallet.FILECOIN_MAINNET:
      return 'FILECOIN'
    case BraveWallet.BITCOIN_MAINNET:
      return isOfframp ? 'BTC' : ''
    default:
      return ''
  }
}

export const formatAsDouble = (value: string): string =>
  // Removes all characters except numbers, commas and decimals
  value.replace(/[^0-9.,]+/g, '')

export const isHttpsUrl = (url: string) => {
  return url.startsWith('https://')
}

export function hasUnicode(str: string) {
  for (let i = 0; i < str.length; i++) {
    if (str.charCodeAt(i) > 127) {
      return true
    }
  }
  return false
}

export function padWithLeadingZeros(string: string) {
  return new Array(5 - string.length).join('0') + string
}

export function unicodeCharEscape(charCode: number) {
  return '\\u' + padWithLeadingZeros(charCode.toString(16))
}

export function unicodeEscape(string: string) {
  return string
    .split('')
    .map(function (char: string) {
      const charCode = char.charCodeAt(0)
      return charCode > 127 ? unicodeCharEscape(charCode) : char
    })
    .join('')
}

/** This prevents there from being more than one space between words. */
export const removeDoubleSpaces = (val: string) => val.replace(/ +(?= )/g, '')

export const getWalletLocationTitle = (location: string) => {
  /** Buy crypto */
  if (location.includes(WalletRoutes.FundWalletPageStart)) {
    return getLocale('braveWalletBuyCryptoButton')
  }
  /** Deposit crypto */
  if (location.includes(WalletRoutes.DepositFundsPageStart)) {
    return getLocale('braveWalletDepositFundsTitle')
  }
  /** Swap */
  if (location === WalletRoutes.Swap) {
    return getLocale('braveWalletSwap')
  }
  if (location === WalletRoutes.Send) {
    return getLocale('braveWalletSend')
  }
  /** Wallet */
  return getLocale('braveWalletTitle')
}

export const endsWithAny = (extensions: string[], url: string) => {
  return extensions.some(function (suffix) {
    return url.endsWith(suffix)
  })
}

export const getNFTTokenStandard = (token: BraveWallet.BlockchainToken) => {
  if (token.isNft && token.coin === BraveWallet.CoinType.SOL) {
    return TokenStandards.SPL
  }
  if (token.isErc721 && token.coin === BraveWallet.CoinType.ETH) {
    return TokenStandards.ERC721
  }
  return ''
}

/**
 * Checks if the component is displayed in a local storybook env
 * Uses loadTimeData for the check,
 * since it is not defined the same in storybook as it is in production
 * There maybe a better way to do this
 * @returns true if loadTimeData returns either a placeholder or an empty value
 */
export const isComponentInStorybook = () => {
  const nftDisplayOrigin =
    loadTimeData.getString('braveWalletNftBridgeUrl') || ''
  return (
    nftDisplayOrigin === 'braveWalletNftBridgeUrl' || nftDisplayOrigin === ''
  )
}

export const IPFS_PROTOCOL = 'ipfs://'

export const isIpfs = (url?: string) => {
  return url?.toLowerCase()?.startsWith(IPFS_PROTOCOL)
}

export const getCid = (ipfsUrl: string) => {
  return ipfsUrl.replace(IPFS_PROTOCOL, '')
}

export const capitalizeFirstLetter = (input: string) => {
  if (input.length === 0) return input
  return input.charAt(0).toUpperCase() + input.slice(1)
}

export const reduceInt = (integerString: string) => {
  if (integerString.length < 7) {
    return integerString
  }

  const firstHalf = integerString.slice(0, 3)
  const secondHalf = integerString.slice(-3)
  const reduced = firstHalf.concat('...', secondHalf)
  return reduced
}
