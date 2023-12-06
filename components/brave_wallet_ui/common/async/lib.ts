// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

import {
  HardwareWalletConnectOpts //
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { BraveWallet } from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import {
  getAssetIdKey,
  getHiddenOrDeletedTokenIdsList,
  isNativeAsset
} from '../../utils/asset-utils'
import {
  makeNativeAssetLogo,
  makeNetworkAsset
} from '../../options/asset-options'
import { getVisibleNetworksList } from '../../utils/api-utils'

import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  LedgerDerivationPaths,
  SolDerivationPaths,
  TrezorDerivationPaths
} from '../hardware/types'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import {
  AllNetworksOption,
  AllNetworksOptionDefault
} from '../../options/network-filter-options'
import {
  AllAccountsOptionUniqueKey,
  applySelectedAccountFilter
} from '../../options/account-filter-options'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'
import {
  IPFS_PROTOCOL,
  isIpfs,
  stripERC20TokenImageURL
} from '../../utils/string-utils'

export const onConnectHardwareWallet = (
  opts: HardwareWalletConnectOpts
): Promise<BraveWallet.HardwareWalletAccount[]> => {
  return new Promise(async (resolve, reject) => {
    const keyring = getHardwareKeyring(
      opts.hardware,
      opts.coin,
      opts.onAuthorized
    )
    const isLedger = keyring instanceof EthereumLedgerBridgeKeyring
    const isTrezor = keyring instanceof TrezorBridgeKeyring
    if ((isLedger || isTrezor) && opts.scheme) {
      const promise = isLedger
        ? keyring.getAccounts(
            opts.startIndex,
            opts.stopIndex,
            opts.scheme as LedgerDerivationPaths
          )
        : keyring.getAccounts(
            opts.startIndex,
            opts.stopIndex,
            opts.scheme as TrezorDerivationPaths
          )

      promise
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (keyring instanceof FilecoinLedgerBridgeKeyring && opts.network) {
      keyring
        .getAccounts(opts.startIndex, opts.stopIndex, opts.network)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (
      keyring instanceof SolanaLedgerBridgeKeyring &&
      opts.network &&
      opts.scheme
    ) {
      keyring
        .getAccounts(
          opts.startIndex,
          opts.stopIndex,
          opts.scheme as SolDerivationPaths
        )
        .then(async (result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            const { braveWalletService } = getAPIProxy()
            const addressesEncoded = await braveWalletService.base58Encode(
              result.payload.map((hardwareAccount) => [
                ...(hardwareAccount.addressBytes || [])
              ])
            )
            for (let i = 0; i < result.payload.length; i++) {
              result.payload[i].address = addressesEncoded.addresses[i]
            }
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    }
  })
}

export async function isStrongPassword(value: string) {
  const apiProxy = getAPIProxy()
  return (await apiProxy.keyringService.isStrongPassword(value)).result
}

export function refreshVisibleTokenInfo(
  targetNetwork?: BraveWallet.NetworkInfo
) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const api = getAPIProxy()
    const { braveWalletService } = api
    const networkList = await getVisibleNetworksList(api)

    async function inner(network: BraveWallet.NetworkInfo) {
      // Get a list of user tokens for each coinType and network.
      const getTokenList = await braveWalletService.getUserAssets(
        network.chainId,
        network.coin
      )

      // Adds a logo and chainId to each token object
      const tokenList = getTokenList.tokens.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
      })) as BraveWallet.BlockchainToken[]

      if (tokenList.length === 0) {
        // user has hidden all tokens for the network
        // we should still include the native asset, but as hidden
        const nativeAsset = makeNetworkAsset(network)
        nativeAsset.visible = false
        return [nativeAsset]
      }

      return tokenList
    }

    const visibleAssets = targetNetwork
      ? await inner(targetNetwork)
      : await mapLimit(
          networkList,
          10,
          async (item: BraveWallet.NetworkInfo) => await inner(item)
        )

    const removedAssetIds = getHiddenOrDeletedTokenIdsList()
    const userVisibleTokensInfo = visibleAssets
      .flat(1)
      .filter((token) => !removedAssetIds.includes(getAssetIdKey(token)))
    await dispatch(WalletActions.setVisibleTokensInfo(userVisibleTokensInfo))
  }
}

export async function getNFTMetadata(token: BraveWallet.BlockchainToken) {
  const { jsonRpcService } = getAPIProxy()
  if (token.coin === BraveWallet.CoinType.ETH) {
    return await jsonRpcService.getERC721Metadata(
      token.contractAddress,
      token.tokenId,
      token.chainId
    )
  } else if (token.coin === BraveWallet.CoinType.SOL) {
    return await jsonRpcService.getSolTokenMetadata(
      token.chainId,
      token.contractAddress
    )
  }

  return undefined
}

export async function isTokenPinningSupported(
  token: BraveWallet.BlockchainToken
) {
  const { braveWalletPinService } = getAPIProxy()
  return await braveWalletPinService.isTokenSupported(token)
}

export function refreshPortfolioFilterOptions() {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { selectedAccountFilter, selectedNetworkFilter } = getState().wallet

    const {
      allAccounts: { accounts }
    } = await getAPIProxy().keyringService.getAllAccounts()

    const networkList = await getVisibleNetworksList(getAPIProxy())

    if (
      selectedNetworkFilter.chainId !== AllNetworksOption.chainId &&
      !networkList.some(
        (network) => network.chainId === selectedNetworkFilter.chainId
      )
    ) {
      dispatch(WalletActions.setSelectedNetworkFilter(AllNetworksOptionDefault))
      window.localStorage.removeItem(
        LOCAL_STORAGE_KEYS.PORTFOLIO_NETWORK_FILTER_OPTION
      )
    }

    if (!applySelectedAccountFilter(accounts, selectedAccountFilter).accounts) {
      dispatch(
        WalletActions.setSelectedAccountFilterItem(AllAccountsOptionUniqueKey)
      )
    }
  }
}

// Checks whether set of urls have ipfs:// scheme or are gateway-like urls
export const areSupportedForPinning = async (urls: string[]) => {
  const results = (
    await mapLimit(
      urls,
      10,
      async (v: string) => await extractIpfsUrl(stripERC20TokenImageURL(v))
    )
  ).flat(1)

  return results.every((result) => result?.startsWith(IPFS_PROTOCOL))
}

// Extracts ipfs:// url from gateway-like url
export const extractIpfsUrl = async (url: string | undefined) => {
  const { braveWalletIpfsService } = getAPIProxy()
  const trimmedUrl = url ? url.trim() : ''
  if (isIpfs(trimmedUrl)) {
    return trimmedUrl
  }
  return (
    (await braveWalletIpfsService.extractIPFSUrlFromGatewayLikeUrl(trimmedUrl))
      ?.ipfsUrl || undefined
  )
}

// Translates ipfs:// url or gateway-like url to the NFT gateway url
export const translateToNftGateway = async (url: string | undefined) => {
  const { braveWalletIpfsService } = getAPIProxy()
  const trimmedUrl = url ? url.trim() : ''
  const testUrl = isIpfs(trimmedUrl)
    ? trimmedUrl
    : await extractIpfsUrl(trimmedUrl)
  return (
    (await braveWalletIpfsService.translateToNFTGatewayURL(testUrl || ''))
      .translatedUrl || trimmedUrl
  )
}

// TODO(apaymyshev): This function should not exist. Backend should be
// responsible in providing correct logo.
export const addLogoToToken = async (token: BraveWallet.BlockchainToken) => {
  const isNative = isNativeAsset(token)

  if (
    (!isNative && !token.logo) ||
    token.logo?.startsWith('data:image/') ||
    token.logo?.startsWith('chrome://erc-token-images/')
  ) {
    // nothing to change
    return token
  }

  const newLogo = isNative
    ? makeNativeAssetLogo(token.symbol, token.chainId)
    : token.logo?.startsWith('ipfs://')
    ? await translateToNftGateway(token.logo)
    : `chrome://erc-token-images/${token.logo}`

  if (token.logo === newLogo) {
    // nothing to change
    return token
  }

  try {
    token.logo = newLogo
    return token
  } catch {
    // the token object was immutable, return a new token object
    return {
      ...token,
      logo: newLogo
    }
  }
}
