// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

export const stripERC20TokenImageURL = (url?: string) =>
  url?.replace('chrome://erc-token-images/', '')

export const toProperCase = (value: string) =>
  value.replace(/\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase())

export const isRemoteImageURL = (url?: string) =>
  url?.startsWith('http://') || url?.startsWith('https://') || url?.startsWith('data:image/') || url?.startsWith('ipfs://')

export const isValidIconExtension = (url?: string) =>
  url?.endsWith('.jpg') || url?.endsWith('.jpeg') || url?.endsWith('.png') || url?.endsWith('.svg') || url?.endsWith('.gif')

export const httpifyIpfsUrl = (url: string | undefined) => {
  const trimmedUrl = url ? url.trim() : ''
  return trimmedUrl.startsWith('ipfs://') ? trimmedUrl.replace('ipfs://', 'https://ipfs.io/ipfs/') : trimmedUrl
}

/**
 * Wyre currently supports the following chains:
 *  bitcoin, ethereum, avalanche(X & C), stellar, algorand, matic, flow
 * @see https://docs.sendwyre.com/reference/system-resource-names-1
 * @param chainId This Id of the chain on which to receive funds
 * @returns a string containing any prefix needed to lookup Wyre assets accross chains
 */
export const getWyreNetworkPrefix = (chainId: string) => {
  switch (chainId) {
    case BraveWallet.POLYGON_MAINNET_CHAIN_ID: return 'M'
    case BraveWallet.AVALANCHE_MAINNET_CHAIN_ID: return 'AVAXC'
    case BraveWallet.MAINNET_CHAIN_ID: return ''
    default: return ''
  }
}

export const getRampNetworkPrefix = (chainId: string) => {
  switch (chainId) {
    case BraveWallet.MAINNET_CHAIN_ID: return ''
    case BraveWallet.AVALANCHE_MAINNET_CHAIN_ID: return 'AVAXC'
    case BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID: return 'BSC'
    case BraveWallet.POLYGON_MAINNET_CHAIN_ID: return 'MATIC'
    case BraveWallet.SOLANA_MAINNET: return 'SOLANA'
    case BraveWallet.OPTIMISM_MAINNET_CHAIN_ID: return 'OPTIMISM'
    case BraveWallet.CELO_MAINNET_CHAIN_ID: return ''
    case BraveWallet.FILECOIN_MAINNET: return 'FILECOIN'
    default: return ''
  }
}

export const formatAsDouble = (value: string): string =>
  // Removes all characters except numbers, commas and decimals
  value.replace(/[^0-9.,]+/g, '')

export const isValidateUrl = (url: string) => {
  const re = /^\s*https?:\/\//
  return re.test(url)
}

export function hasUnicode (str: string) {
  for (let i = 0; i < str.length; i++) {
    if (str.charCodeAt(i) > 127) {
      return true
    }
  }
  return false
}

export function padWithLeadingZeros (string: string) {
  return new Array(5 - string.length).join('0') + string
}

export function unicodeCharEscape (charCode: number) {
  return '\\u' + padWithLeadingZeros(charCode.toString(16))
}

export function unicodeEscape (string: string) {
  return string.split('')
    .map(function (char: string) {
      const charCode = char.charCodeAt(0)
      return charCode > 127 ? unicodeCharEscape(charCode) : char
    })
    .join('')
}
