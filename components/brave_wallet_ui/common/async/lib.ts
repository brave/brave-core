// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

import {
  HardwareWalletConnectOpts //
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { BraveWallet } from '../../constants/types'

// Utils
import { isNativeAsset } from '../../utils/asset-utils'
import { makeNativeAssetLogo } from '../../options/asset-options'

import getAPIProxy from './bridge'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  LedgerDerivationPaths,
  SolDerivationPaths,
  TrezorDerivationPaths
} from '../hardware/types'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
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
