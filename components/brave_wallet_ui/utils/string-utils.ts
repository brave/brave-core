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

export const sanitizeImageURL = (url: string): string =>
  isRemoteImageURL(url)
    ? `chrome://image?url=${encodeURIComponent(url)}&staticEncode=true`
    : url

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

export type HiddenSignMessageCharacter =
  | 'nullByte'
  | 'newline'
  | 'nonAscii'
  | 'controlCharacter'

/** Line breaks, including CR, VT, and FF, which can pad a preview like LF. */
function isNewlineCharacter(charCode: number) {
  return (
    charCode === 0x0a
    || charCode === 0x0b
    || charCode === 0x0c
    || charCode === 0x0d
  )
}

/**
 * ASCII that cannot be rendered directly: the rest of the C0 controls, and DEL.
 * Space (0x20) is excluded.
 */
function isControlCharacter(charCode: number) {
  return (
    (charCode < 0x20 && charCode !== 0 && !isNewlineCharacter(charCode))
    || charCode === 0x7f
  )
}

/**
 * Characters that can make a sign-message preview disagree with the bytes
 * being signed: null bytes, newlines, other non-printable ASCII, and non-ASCII.
 */
export function getHiddenSignMessageCharacters(
  ...values: Array<string | undefined>
): HiddenSignMessageCharacter[] {
  let nullByte = false
  let newline = false
  let nonAscii = false
  let controlCharacter = false
  for (const value of values) {
    if (!value) {
      continue
    }
    for (let i = 0; i < value.length; i++) {
      const charCode = value.charCodeAt(i)
      if (charCode === 0) {
        nullByte = true
      } else if (isNewlineCharacter(charCode)) {
        newline = true
      } else if (charCode > 127) {
        nonAscii = true
      } else if (isControlCharacter(charCode)) {
        controlCharacter = true
      }
    }
  }

  const found: HiddenSignMessageCharacter[] = []
  if (nullByte) {
    found.push('nullByte')
  }
  if (newline) {
    found.push('newline')
  }
  if (nonAscii) {
    found.push('nonAscii')
  }
  if (controlCharacter) {
    found.push('controlCharacter')
  }
  return found
}

export function hasUnicode(str: string) {
  return hasUnexpectedSignMessageCharacters([str])
}

function collectJsonStrings(value: unknown, out: string[]) {
  if (typeof value === 'string') {
    out.push(value)
    return
  }
  if (Array.isArray(value)) {
    for (const item of value) {
      collectJsonStrings(item, out)
    }
    return
  }
  if (value && typeof value === 'object') {
    for (const [key, item] of Object.entries(value)) {
      out.push(key)
      collectJsonStrings(item, out)
    }
  }
}

/**
 * EIP-712 domain and message are re-encoded as JSON before display. That
 * encoding writes a null as \u0000 and a newline as \n, so a raw-byte scan
 * of the JSON text misses them. Scan the decoded values as well.
 */
export function getTypedDataHiddenSignMessageCharacters(
  ...jsonValues: Array<string | undefined>
) {
  const messages: string[] = []
  for (const json of jsonValues) {
    if (!json) {
      continue
    }
    messages.push(json)
    try {
      collectJsonStrings(JSON.parse(json), messages)
    } catch {
      // Not JSON. The raw text was already included above.
    }
  }
  return getHiddenSignMessageCharacters(...messages)
}

/** True when a sign message should show the unexpected-characters warning. */
export function hasUnexpectedSignMessageCharacters(
  messages: Array<string | undefined>,
  decodeJson = false,
) {
  const characters = decodeJson
    ? getTypedDataHiddenSignMessageCharacters(...messages)
    : getHiddenSignMessageCharacters(...messages)
  return characters.length > 0
}

export function padWithLeadingZeros(string: string) {
  return new Array(5 - string.length).join('0') + string
}

export function unicodeCharEscape(charCode: number) {
  return '\\u' + padWithLeadingZeros(charCode.toString(16))
}

export function unicodeEscape(value: string) {
  return value
    .split('')
    .map((char: string) => {
      const charCode = char.charCodeAt(0)
      // A literal backslash must not look like an escape we insert below.
      if (charCode === 0x5c) {
        return '\\\\'
      }
      if (charCode === 0) {
        return '\\0'
      }
      // Keep the line break, and prefix a visible marker so a newline byte
      // is not mistaken for ordinary text wrapping. CR, VT, and FF get the
      // same treatment.
      if (charCode === 0x0a) {
        return '\\n\n'
      }
      if (charCode === 0x0d) {
        return '\\r\n'
      }
      if (charCode === 0x0b) {
        return '\\v\n'
      }
      if (charCode === 0x0c) {
        return '\\f\n'
      }
      if (charCode < 0x20 || charCode === 0x7f || charCode > 127) {
        return unicodeCharEscape(charCode)
      }
      return char
    })
    .join('')
}

/** Display-only. Callers must still sign the original message bytes. */
export function formatSignMessageForDisplay(
  message: string,
  showFormatted: boolean,
) {
  return showFormatted ? unicodeEscape(message) : message
}

/**
 * Typed-data JSON already writes a null as \u0000 and a newline as \n. Decode
 * each string, then apply the same ASCII markers used for other sign requests,
 * so the formatted view is distinct from that JSON text.
 */
export function formatTypedDataForDisplay(
  message: string,
  showFormatted: boolean,
) {
  if (!showFormatted) {
    return message
  }
  try {
    JSON.parse(message)
  } catch {
    return unicodeEscape(message)
  }
  return message.replace(/"(?:\\.|[^"\\])*"/g, (literal) => {
    try {
      const decoded = JSON.parse(literal)
      if (typeof decoded !== 'string') {
        return literal
      }
      return '"' + unicodeEscape(decoded).replace(/"/g, '\\"') + '"'
    } catch {
      return literal
    }
  })
}

/** This prevents there from being more than one space between words. */
export const removeDoubleSpaces = (val: string) => val.replace(/ +(?= )/g, '')

export const getWalletLocationTitle = (location: string) => {
  /** Buy crypto */
  if (location.includes(WalletRoutes.BuyPageStart)) {
    return getLocale(S.BRAVE_WALLET_BUY_CRYPTO_BUTTON)
  }
  /** Deposit crypto */
  if (location.includes(WalletRoutes.DepositPageStart)) {
    return getLocale(S.BRAVE_WALLET_DEPOSIT_FUNDS_TITLE)
  }
  /** Swap */
  if (location === WalletRoutes.Swap) {
    return getLocale(S.BRAVE_WALLET_SWAP)
  }
  if (location === WalletRoutes.Send) {
    return getLocale(S.BRAVE_WALLET_SEND)
  }
  /** Wallet */
  return getLocale(S.BRAVE_WALLET_TITLE)
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
