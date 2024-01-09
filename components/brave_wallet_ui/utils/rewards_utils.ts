// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Assets
import {
  UpholdIcon,
  GeminiIcon,
  ZebpayIcon,
  BitflyerIcon
} from '../assets/svg-icons/provider_icons'

// Utils
import { getLocale } from '../../common/locale'

// Types
import { externalWalletProviders } from '../common/async/brave_rewards_api_proxy'
import { BraveWallet } from '../constants/types'
import { ExternalWalletProvider } from '../../brave_rewards/resources/shared/lib/external_wallet'

export const getRewardsProviderName = (provider?: string) => {
  if (!provider) {
    return ''
  }

  const capitalized = provider.charAt(0).toUpperCase() + provider.slice(1)
  const localeString = `braveWallet${capitalized}`
  const foundLocale = getLocale(localeString)
  // If getLocale returns the string it was passed, that means
  // no localization string was found. So we return the
  // provider in that case.
  return foundLocale === localeString ? provider : foundLocale
}

export const getRewardsAccountName = (provider?: string) => {
  if (!provider) {
    return ''
  }
  return getLocale('braveWalletRewardsAccount').replace(
    '$1',
    getRewardsProviderName(provider)
  )
}

export const getRewardsTokenDescription = (
  provider: ExternalWalletProvider | null
) => {
  if (!provider) {
    return ''
  }
  return getLocale('braveWalletBraveRewardsDescription').replace(
    '$1',
    getRewardsProviderName(provider)
  )
}

export const getNormalizedExternalRewardsWallet = (
  externalRewardsProvider?: ExternalWalletProvider | null
): BraveWallet.AccountInfo | undefined => {
  if (!externalRewardsProvider) {
    return undefined
  }
  return {
    accountId: {
      address: '0x',
      bitcoinAccountIndex: 0,
      coin: BraveWallet.CoinType.ETH,
      keyringId: 0,
      kind: 0,
      uniqueKey: externalRewardsProvider
    },
    address: '0x',
    hardware: undefined,
    name: getRewardsAccountName(externalRewardsProvider)
  }
}

export const getNormalizedExternalRewardsNetwork = (
  externalRewardsProvider?: ExternalWalletProvider | null
): BraveWallet.NetworkInfo | undefined => {
  if (!externalRewardsProvider) {
    return undefined
  }
  return {
    activeRpcEndpointIndex: 0,
    blockExplorerUrls: [''],
    chainId: externalRewardsProvider,
    chainName: getRewardsProviderName(externalRewardsProvider),
    coin: 0,
    supportedKeyrings: [],
    decimals: 0,
    iconUrls: [],
    isEip1559: true,
    rpcEndpoints: [],
    symbol: externalRewardsProvider,
    symbolName: externalRewardsProvider
  }
}

export const getIsRewardsAccount = (
  accountId?: Pick<BraveWallet.AccountId, 'uniqueKey'>
) => {
  if (!accountId) {
    return false
  }
  return externalWalletProviders.includes(accountId.uniqueKey)
}

export const getIsRewardsNetwork = (
  network?: Pick<BraveWallet.NetworkInfo, 'chainId'>
) => {
  if (!network) {
    return false
  }
  return externalWalletProviders.includes(network.chainId)
}

export const getIsRewardsToken = (
  token?: Pick<BraveWallet.BlockchainToken, 'chainId'>
) => {
  if (!token) {
    return false
  }
  return externalWalletProviders.includes(token.chainId)
}

export const getRewardsProviderIcon = (
  provider: ExternalWalletProvider | null
) => {
  switch (provider) {
    case 'bitflyer':
      return BitflyerIcon
    case 'gemini':
      return GeminiIcon
    case 'uphold':
      return UpholdIcon
    case 'zebpay':
      return ZebpayIcon
    default:
      return ''
  }
}

export const getRewardsProviderBackground = (
  provider: ExternalWalletProvider | null
) => {
  switch (provider) {
    case 'bitflyer':
      return 'rgb(52, 152, 212)'
    case 'gemini':
      return 'rgb(97, 217, 245)'
    case 'uphold':
      return 'rgb(73, 204, 104)'
    case 'zebpay':
      return 'rgb(3, 116, 242)'
    default:
      return ''
  }
}

export const getRewardsBATToken = (
  provider: ExternalWalletProvider | undefined
): BraveWallet.BlockchainToken | undefined => {
  if (!provider) {
    return undefined
  }
  return {
    contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: 'chrome://erc-token-images/bat.png',
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: provider
  }
}
