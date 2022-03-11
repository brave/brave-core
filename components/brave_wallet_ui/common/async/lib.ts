// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { assert } from 'chrome://resources/js/assert.m.js'
import { CoinType } from '@glif/filecoin-address'
import {
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  AccountTransactions,
  BraveWallet,
  WalletAccountType,
  AccountInfo,
  BraveKeyrings,
  GetBlockchainTokenInfoReturnInfo
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { GetNetworkInfo } from '../../utils/network-utils'
import { GetTokenParam, GetFlattenedAccountBalances } from '../../utils/api-utils'
import Amount from '../../utils/amount'

import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import { GetAccountsHardwareOperationResult } from '../hardware/types'
import LedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import FilecoinLedgerKeyring from '../hardware/ledgerjs/filecoin_ledger_keyring'

export const getERC20Allowance = (
  contractAddress: string,
  ownerAddress: string,
  spenderAddress: string
): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const service = getAPIProxy().jsonRpcService
    const result = await service.getERC20TokenAllowance(
      contractAddress,
      ownerAddress,
      spenderAddress
    )

    if (result.error === BraveWallet.ProviderError.kSuccess) {
      resolve(result.allowance)
    } else {
      reject()
    }
  })
}

export const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Promise<BraveWallet.HardwareWalletAccount[]> => {
  return new Promise(async (resolve, reject) => {
    const keyring = getHardwareKeyring(opts.hardware, opts.coin)
    if (keyring instanceof LedgerBridgeKeyring || keyring instanceof TrezorBridgeKeyring) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.scheme)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (keyring instanceof FilecoinLedgerKeyring) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, CoinType.TEST)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    }
  })
}

export const getBalance = (address: string): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const { jsonRpcService } = getAPIProxy()
    const chainId = await jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
    const result = await jsonRpcService.getBalance(address, BraveWallet.CoinType.ETH, chainId.chainId)
    if (result.error === BraveWallet.ProviderError.kSuccess) {
      resolve(Amount.normalize(result.balance))
    } else {
      reject()
    }
  })
}

export async function getChecksumEthAddress (value: string) {
  const { keyringService } = getAPIProxy()
  return (await keyringService.getChecksumEthAddress(value))
}

export async function isStrongPassword (value: string) {
  const apiProxy = getAPIProxy()
  return (await apiProxy.keyringService.isStrongPassword(value)).result
}

export async function findENSAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.ensGetEthAddr(address)
}

export async function findUnstoppableDomainAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.unstoppableDomainsGetEthAddr(address)
}

export async function getBlockchainTokenInfo (contractAddress: string): Promise<GetBlockchainTokenInfoReturnInfo> {
  const apiProxy = getAPIProxy()
  return (await apiProxy.assetRatioService.getTokenInfo(contractAddress))
}

export async function findHardwareAccountInfo (address: string): Promise<AccountInfo | false> {
  const apiProxy = getAPIProxy()
  const result = await apiProxy.walletHandler.getWalletInfo()
  for (const account of result.accountInfos) {
    if (!account.hardware) {
      continue
    }
    if (account.address === address) {
      return account
    }
  }
  return false
}

export async function getBuyAssetUrl (address: string, symbol: string, amount: string) {
  const { blockchainRegistry } = getAPIProxy()
  return (await blockchainRegistry.getBuyUrl(BraveWallet.MAINNET_CHAIN_ID, address, symbol, amount)).url
}

export async function getBuyAssets () {
  const { blockchainRegistry } = getAPIProxy()
  return (await blockchainRegistry.getBuyTokens(BraveWallet.MAINNET_CHAIN_ID)).tokens
}

export function getKeyringIdFromCoin (coin: BraveWallet.CoinType): BraveKeyrings {
  if (coin === BraveWallet.CoinType.FIL) {
    return BraveWallet.FILECOIN_KEYRING_ID
  }
  assert(coin === BraveWallet.CoinType.ETH)
  return BraveWallet.DEFAULT_KEYRING_ID
}

export async function getKeyringIdFromAddress (address: string): Promise<string> {
  const apiProxy = getAPIProxy()
  const result = await apiProxy.walletHandler.getWalletInfo()
  for (const account of result.accountInfos) {
    if (account.address === address) {
      return getKeyringIdFromCoin(account.coin)
    }
  }
  return getKeyringIdFromCoin(BraveWallet.CoinType.ETH)
}

export async function getIsSwapSupported (network: BraveWallet.NetworkInfo): Promise<boolean> {
  const { swapService } = getAPIProxy()
  return (await swapService.isSwapSupported(network.chainId)).result
}

export function refreshVisibleTokenInfo (currentNetwork: BraveWallet.NetworkInfo) {
  return async (dispatch: Dispatch) => {
    const { braveWalletService } = getAPIProxy()

    // Selected Network's Native Asset
    const nativeAsset: BraveWallet.BlockchainToken = {
      contractAddress: '',
      decimals: currentNetwork.decimals,
      isErc20: false,
      isErc721: false,
      logo: currentNetwork.iconUrls[0] ?? '',
      name: currentNetwork.symbolName,
      symbol: currentNetwork.symbol,
      visible: false,
      tokenId: '',
      coingeckoId: ''
    }

    const visibleTokensInfo = await braveWalletService.getUserAssets(currentNetwork.chainId)
    const visibleAssets: BraveWallet.BlockchainToken[] = visibleTokensInfo.tokens.length === 0 ? [nativeAsset] : visibleTokensInfo.tokens
    await dispatch(WalletActions.setVisibleTokensInfo(visibleAssets))
  }
}

export function refreshBalances () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { jsonRpcService } = getAPIProxy()
    const { wallet: { accounts, userVisibleTokensInfo, selectedNetwork } } = getState()

    const getBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const balanceInfo = await jsonRpcService.getBalance(account.address, account.coin, selectedNetwork.chainId)
      return balanceInfo
    }))
    await dispatch(WalletActions.nativeAssetBalancesUpdated({
      balances: getBalanceReturnInfos
    }))

    const visibleTokens = userVisibleTokensInfo.filter(asset => asset.contractAddress !== '')

    const getBlockchainTokenBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      return Promise.all(visibleTokens.map(async (token) => {
        if (token.isErc721) {
          return jsonRpcService.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address, selectedNetwork.chainId)
        }
        return jsonRpcService.getERC20TokenBalance(token.contractAddress, account.address, selectedNetwork.chainId)
      }))
    }))

    await dispatch(WalletActions.tokenBalancesUpdated({
      balances: getBlockchainTokenBalanceReturnInfos
    }))
  }
}

export function refreshPrices () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { assetRatioService } = getAPIProxy()
    const { wallet: { accounts, selectedPortfolioTimeline, selectedNetwork, userVisibleTokensInfo, defaultCurrencies } } = getState()

    const defaultFiatCurrency = defaultCurrencies.fiat.toLowerCase()
    // Fetch native asset (ETH) price
    const getNativeAssetPrice = await assetRatioService.getPrice([selectedNetwork.symbol.toLowerCase()], [defaultFiatCurrency], selectedPortfolioTimeline)
    const nativeAssetPrice = getNativeAssetPrice.success ? getNativeAssetPrice.values.find((i) => i.toAsset === defaultFiatCurrency)?.price ?? '' : ''

    // Update Token Prices
    if (!userVisibleTokensInfo) {
      return
    }

    const getTokenPrices = await Promise.all(GetFlattenedAccountBalances(accounts, userVisibleTokensInfo).map(async (token) => {
      const emptyPrice = {
        fromAsset: token.token.symbol,
        toAsset: defaultFiatCurrency,
        price: '',
        assetTimeframeChange: ''
      }

      // If a tokens balance is 0 we do not make an unnecessary api call for the price of that token
      const price = token.balance > 0 && token.token.isErc20
        ? await assetRatioService.getPrice([GetTokenParam(selectedNetwork, token.token)], [defaultFiatCurrency], selectedPortfolioTimeline)
        : { values: [{ ...emptyPrice, price: '0' }], success: true }

      const tokenPrice = {
        ...price.values[0],
        fromAsset: token.token.symbol.toLowerCase()
      }
      return price.success ? tokenPrice : emptyPrice
    }))

    await dispatch(WalletActions.pricesUpdated({
      success: true,
      values: [
        {
          fromAsset: selectedNetwork.symbol.toLowerCase(),
          toAsset: defaultFiatCurrency,
          price: nativeAssetPrice,
          assetTimeframeChange: ''
        },
        ...getTokenPrices
      ]
    }))
  }
}

export function refreshTokenPriceHistory (selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { assetRatioService } = apiProxy

    const { wallet: { accounts, defaultCurrencies, selectedNetwork, userVisibleTokensInfo } } = getState()

    // If a tokens balance is 0 we do not make an unnecessary api call for price history of that token
    const priceHistory = await Promise.all(GetFlattenedAccountBalances(accounts, userVisibleTokensInfo)
      .filter(({ token, balance }) => !token.isErc721 && balance > 0)
      .map(async ({ token }) => ({
        contractAddress: token.contractAddress,
        history: await assetRatioService.getPriceHistory(
          GetTokenParam(selectedNetwork, token), defaultCurrencies.fiat.toLowerCase(), selectedPortfolioTimeline
        )
      }))
    )

    const priceHistoryWithBalances = accounts.map((account) => {
      return userVisibleTokensInfo
        .filter((token) => !token.isErc721)
        .map((token) => {
          const balance = token.contractAddress
            ? account.tokenBalanceRegistry[token.contractAddress.toLowerCase()]
            : account.balance
          return {
            token,
            balance: balance || '0',
            history: priceHistory.find((t) => token.contractAddress === t.contractAddress)?.history ?? { success: true, values: [] }
          }
        })
    })

    dispatch(WalletActions.portfolioPriceHistoryUpdated(priceHistoryWithBalances))
  }
}

export function refreshTransactionHistory (address?: string) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { txService } = apiProxy

    const { wallet: { accounts, transactions } } = getState()

    const accountsToUpdate = address !== undefined
      ? accounts.filter(account => account.address === address)
      : accounts

    const freshTransactions: AccountTransactions = await accountsToUpdate.reduce(
      async (acc, account) => acc.then(async (obj) => {
        const { transactionInfos } = await txService.getAllTransactionInfo(BraveWallet.CoinType.ETH, account.address)
        obj[account.address] = transactionInfos
        return obj
      }), Promise.resolve({}))

    dispatch(WalletActions.setAccountTransactions({
      ...transactions,
      ...freshTransactions
    }))
  }
}

export function refreshNetworkInfo () {
  return async (dispatch: Dispatch) => {
    const apiProxy = getAPIProxy()
    const { jsonRpcService } = apiProxy

    const networkList = await jsonRpcService.getAllNetworks(BraveWallet.CoinType.ETH)
    dispatch(WalletActions.setAllNetworks(networkList))
    const chainId = await jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
    const currentNetwork = GetNetworkInfo(chainId.chainId, networkList.networks)
    dispatch(WalletActions.setNetwork(currentNetwork))
    return currentNetwork
  }
}

export function refreshKeyringInfo () {
  return async (dispatch: Dispatch) => {
    const apiProxy = getAPIProxy()
    const { keyringService, walletHandler } = apiProxy

    const walletInfoBase = await walletHandler.getWalletInfo()
    const walletInfo = { ...walletInfoBase, visibleTokens: [], selectedAccount: '' }

    // Get/Set selectedAccount
    if (!walletInfo.isWalletCreated) {
      dispatch(WalletActions.initialized(walletInfo))
      return
    }

    // Get selectedAccountAddress
    const getSelectedAccount = await keyringService.getSelectedAccount(BraveWallet.CoinType.ETH)
    const selectedAddress = getSelectedAccount.address

    // Fallback account address if selectedAccount returns null
    const fallbackAccount = walletInfo.accountInfos[0]

    // If selectedAccount is null will setSelectedAccount to fallback address
    if (!selectedAddress) {
      await keyringService.setSelectedAccount(fallbackAccount.address, fallbackAccount.coin)
      walletInfo.selectedAccount = fallbackAccount.address
    } else {
      // If a user has already created an wallet but then chooses to restore
      // a different wallet, getSelectedAccount still returns the previous wallets
      // selected account.
      // This check looks to see if the returned selectedAccount exist in the accountInfos
      // payload, if not it will setSelectedAccount to the fallback address
      if (!walletInfo.accountInfos.find((account) => account.address.toLowerCase() === selectedAddress?.toLowerCase())) {
        walletInfo.selectedAccount = fallbackAccount.address
        await keyringService.setSelectedAccount(fallbackAccount.address, fallbackAccount.coin)
      } else {
        walletInfo.selectedAccount = selectedAddress
      }
    }

    dispatch(WalletActions.initialized(walletInfo))
  }
}

export function refreshSitePermissions () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { braveWalletService } = apiProxy

    const { wallet: { accounts, activeOrigin } } = getState()

    // Get a list of accounts with permissions of the active origin
    const getAllPermissions = await Promise.all(accounts.map(async (account) => {
      const result = await braveWalletService.hasEthereumPermission(activeOrigin, account.address)
      if (result.hasPermission) {
        return account
      }

      return undefined
    }))
    const accountsWithPermission: Array<WalletAccountType | undefined> = getAllPermissions.filter((account) => account !== undefined)
    dispatch(WalletActions.setSitePermissions({ accounts: accountsWithPermission }))
  }
}

/**
 * Check if the keyring associated with the given account AND the network
 * support the EIP-1559 fee market for paying gas fees.
 *
 * This method can also be used to determine if the given parameters support
 * EVM Type-2 transactions. The return value is always false for non-EVM
 * networks.
 *
 * @param {WalletAccountType} account
 * @param {BraveWallet.NetworkInfo} network
 * @returns {boolean} Returns a boolean result indicating EIP-1559 support.
 */
export function hasEIP1559Support (account: WalletAccountType, network: BraveWallet.NetworkInfo) {
  let keyringSupportsEIP1559
  switch (account.accountType) {
    case 'Primary':
    case 'Secondary':
    case 'Ledger':
    case 'Trezor':
      keyringSupportsEIP1559 = true
      break
    default:
      keyringSupportsEIP1559 = false
  }

  return keyringSupportsEIP1559 && (network.data?.ethData?.isEip1559 ?? false)
}
