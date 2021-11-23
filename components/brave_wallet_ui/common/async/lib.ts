// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  HardwareWalletAccount,
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { formatBalance } from '../../utils/format-balances'
import {
  AccountTransactions,
  AssetPriceTimeframe,
  EthereumChain,
  ERCToken,
  WalletAccountType
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { GetNetworkInfo } from '../../utils/network-utils'
import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import LedgerBridgeKeyring from '../../common/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/trezor/trezor_bridge_keyring'
import { GetAccountsHardwareOperationResult } from '../hardware_operations'

export const getERC20Allowance = (
  contractAddress: string,
  ownerAddress: string,
  spenderAddress: string
): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const controller = getAPIProxy().ethJsonRpcController
    const result = await controller.getERC20TokenAllowance(
      contractAddress,
      ownerAddress,
      spenderAddress
    )

    if (result.success) {
      resolve(result.allowance)
    } else {
      reject()
    }
  })
}

export const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Promise<HardwareWalletAccount[]> => {
  return new Promise(async (resolve, reject) => {
    const apiProxy = getAPIProxy()
    const keyring = await apiProxy.getKeyringsByType(opts.hardware)
    if (keyring instanceof LedgerBridgeKeyring || keyring instanceof TrezorBridgeKeyring) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.scheme)
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
    const controller = getAPIProxy().ethJsonRpcController
    const result = await controller.getBalance(address)
    if (result.success) {
      resolve(formatBalance(result.balance, 18))
    } else {
      reject()
    }
  })
}

export async function findENSAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.ethJsonRpcController.ensGetEthAddr(address)
}

export async function findUnstoppableDomainAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.ethJsonRpcController.unstoppableDomainsGetEthAddr(address)
}

export async function findHardwareAccountInfo (address: string) {
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
  return null
}

export function refreshBalances (currentNetwork: EthereumChain) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { wallet: { accounts } } = getState()

    const { braveWalletService, ethJsonRpcController } = apiProxy

    const visibleTokensInfo = await braveWalletService.getUserAssets(currentNetwork.chainId)

    // Selected Network's Native Asset
    const nativeAsset: ERCToken = {
      contractAddress: '',
      decimals: currentNetwork.decimals,
      isErc20: false,
      isErc721: false,
      logo: currentNetwork.iconUrls[0] ?? '',
      name: currentNetwork.symbolName,
      symbol: currentNetwork.symbol,
      visible: false,
      tokenId: ''
    }

    const visibleTokens: ERCToken[] = visibleTokensInfo.tokens.length === 0 ? [nativeAsset] : visibleTokensInfo.tokens
    await dispatch(WalletActions.setVisibleTokensInfo(visibleTokens))

    const getBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const balanceInfo = await ethJsonRpcController.getBalance(account.address)
      return balanceInfo
    }))
    const balancesAndPrice = {
      usdPrice: '',
      balances: getBalanceReturnInfos
    }
    await dispatch(WalletActions.nativeAssetBalancesUpdated(balancesAndPrice))

    const getERCTokenBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      return Promise.all(visibleTokens.map(async (token) => {
        if (token.isErc721) {
          return ethJsonRpcController.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address)
        }
        return ethJsonRpcController.getERC20TokenBalance(token.contractAddress, account.address)
      }))
    }))

    const tokenBalancesAndPrices = {
      balances: getERCTokenBalanceReturnInfos,
      prices: { success: true, values: [] }
    }
    await dispatch(WalletActions.tokenBalancesUpdated(tokenBalancesAndPrices))
  }
}

export function refreshPrices () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { wallet: { accounts, selectedPortfolioTimeline, selectedNetwork, userVisibleTokensInfo } } = getState()

    const { assetRatioController } = apiProxy

    // Update ETH Balances
    const getNativeAssetPrice = await assetRatioController.getPrice([selectedNetwork.symbol.toLowerCase()], ['usd'], selectedPortfolioTimeline)
    const nativeAssetPrice = getNativeAssetPrice.success ? getNativeAssetPrice.values.find((i) => i.toAsset === 'usd')?.price ?? '' : ''
    const getBalanceReturnInfos = accounts.map((account) => {
      const balanceInfo = {
        success: true,
        balance: account.balance
      }
      return balanceInfo
    })
    const balancesAndPrice = {
      usdPrice: nativeAssetPrice,
      balances: getBalanceReturnInfos
    }

    await dispatch(WalletActions.nativeAssetBalancesUpdated(balancesAndPrice))

    // Update Token Balances
    if (!userVisibleTokensInfo) {
      return
    }

    const getTokenPrices = await Promise.all(userVisibleTokensInfo.map(async (token) => {
      const emptyPrice = {
        assetTimeframeChange: '',
        fromAsset: token.symbol,
        price: '',
        toAsset: 'usd'
      }
      const price = await assetRatioController.getPrice([token.symbol.toLowerCase()], ['usd'], selectedPortfolioTimeline)
      return price.success ? price.values[0] : emptyPrice
    }))

    const getERCTokenBalanceReturnInfos = accounts.map((account) => {
      return account.tokens.map((token) => {
        const balanceInfo = {
          success: true,
          balance: token.assetBalance
        }
        return balanceInfo
      })
    })

    const tokenBalancesAndPrices = {
      balances: getERCTokenBalanceReturnInfos,
      prices: { success: true, values: getTokenPrices }
    }

    await dispatch(WalletActions.tokenBalancesUpdated(tokenBalancesAndPrices))
  }
}

export function refreshTokenPriceHistory (selectedPortfolioTimeline: AssetPriceTimeframe) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { assetRatioController } = apiProxy

    const { wallet: { accounts } } = getState()

    const result = await Promise.all(accounts.map(async (account) => {
      return Promise.all(account.tokens.filter((t) => !t.asset.isErc721).map(async (token) => {
        return {
          token: token,
          history: await assetRatioController.getPriceHistory(
            token.asset.symbol.toLowerCase(), selectedPortfolioTimeline
          )
        }
      }))
    }))

    dispatch(WalletActions.portfolioPriceHistoryUpdated(result))
  }
}

export function refreshTransactionHistory (address?: string) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { ethTxController } = apiProxy

    const { wallet: { accounts, transactions } } = getState()

    const accountsToUpdate = address !== undefined
      ? accounts.filter(account => account.address === address)
      : accounts

    const freshTransactions: AccountTransactions = await accountsToUpdate.reduce(
      async (acc, account) => acc.then(async (obj) => {
        const { transactionInfos } = await ethTxController.getAllTransactionInfo(account.address)
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
    const { ethJsonRpcController } = apiProxy

    const networkList = await ethJsonRpcController.getAllNetworks()
    dispatch(WalletActions.setAllNetworks(networkList))
    const chainId = await ethJsonRpcController.getChainId()
    const currentNetwork = GetNetworkInfo(chainId.chainId, networkList.networks)
    dispatch(WalletActions.setNetwork(currentNetwork))
    return currentNetwork
  }
}

export function refreshKeyringInfo () {
  return async (dispatch: Dispatch) => {
    const apiProxy = getAPIProxy()
    const { keyringController, walletHandler } = apiProxy

    const walletInfoBase = await walletHandler.getWalletInfo()
    const walletInfo = { ...walletInfoBase, visibleTokens: [], selectedAccount: '' }

    // Get/Set selectedAccount
    if (!walletInfo.isWalletCreated) {
      dispatch(WalletActions.initialized(walletInfo))
      return
    }

    // Get selectedAccountAddress
    const getSelectedAccount = await keyringController.getSelectedAccount()
    const selectedAddress = getSelectedAccount.address

    // Fallback account address if selectedAccount returns null
    const fallbackAddress = walletInfo.accountInfos[0].address

    // If selectedAccount is null will setSelectedAccount to fallback address
    if (!selectedAddress) {
      await keyringController.setSelectedAccount(fallbackAddress)
      walletInfo.selectedAccount = fallbackAddress
    } else {
      // If a user has already created an wallet but then chooses to restore
      // a different wallet, getSelectedAccount still returns the previous wallets
      // selected account.
      // This check looks to see if the returned selectedAccount exist in the accountInfos
      // payload, if not it will setSelectedAccount to the fallback address
      if (!walletInfo.accountInfos.find((account) => account.address.toLowerCase() === selectedAddress?.toLowerCase())) {
        walletInfo.selectedAccount = fallbackAddress
        await keyringController.setSelectedAccount(fallbackAddress)
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
