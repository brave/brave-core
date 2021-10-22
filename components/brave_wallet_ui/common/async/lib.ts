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
  AssetPriceTimeframe,
  EthereumChain,
  TokenInfo,
  WalletAccountType
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { GetNetworkInfo } from '../../utils/network-utils'

import getAPIProxy from './bridge'
import { Dispatch, State } from './types'

export const getERC20Allowance = (
  contractAddress: string,
  ownerAddress: string,
  spenderAddress: string
): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const controller = (await getAPIProxy()).ethJsonRpcController
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
    const apiProxy = await getAPIProxy()
    const keyring = await apiProxy.getKeyringsByType(opts.hardware)
    keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.scheme)
      .then(async (accounts: HardwareWalletAccount[]) => {
        resolve(accounts)
      })
      .catch(reject)
  })
}

export const getBalance = (address: string): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const controller = (await getAPIProxy()).ethJsonRpcController
    const result = await controller.getBalance(address)
    if (result.success) {
      resolve(formatBalance(result.balance, 18))
    } else {
      reject()
    }
  })
}

export async function findENSAddress (address: string) {
  const apiProxy = await getAPIProxy()
  return apiProxy.ethJsonRpcController.ensGetEthAddr(address)
}

export async function findUnstoppableDomainAddress (address: string) {
  const apiProxy = await getAPIProxy()
  return apiProxy.ethJsonRpcController.unstoppableDomainsGetEthAddr(address)
}

export async function findHardwareAccountInfo (address: string) {
  const apiProxy = await getAPIProxy()
  const result = await apiProxy.walletHandler.getWalletInfo()
  for (const account of result.accountInfos) {
    if (!account.hardware) {
      continue
    }
    if (account.address.toLowerCase() === address) {
      return account
    }
  }
  return null
}

export function refreshBalancesAndPrices (currentNetwork: EthereumChain) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = await getAPIProxy()
    const { wallet: { accounts, selectedPortfolioTimeline } } = getState()

    const { braveWalletService, assetRatioController, ethJsonRpcController } = apiProxy
    const visibleTokensInfo = await braveWalletService.getUserAssets(currentNetwork.chainId)

    // Selected Network's Base Asset
    const initialToken: TokenInfo[] = [{
      contractAddress: '',
      decimals: currentNetwork.decimals,
      isErc20: false,
      isErc721: false,
      logo: '',
      name: currentNetwork.symbolName,
      symbol: currentNetwork.symbol,
      visible: false
    }]

    const visibleTokens: TokenInfo[] = visibleTokensInfo.tokens.length === 0 ? initialToken : visibleTokensInfo.tokens
    await dispatch(WalletActions.setVisibleTokensInfo(visibleTokens))

    // Update ETH Balances
    const getEthPrice = await assetRatioController.getPrice(['eth'], ['usd'], selectedPortfolioTimeline)
    const ethPrice = getEthPrice.success ? getEthPrice.values.find((i) => i.toAsset === 'usd')?.price ?? '0' : '0'
    const getBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const balanceInfo = await ethJsonRpcController.getBalance(account.address)
      return balanceInfo
    }))
    const balancesAndPrice = {
      usdPrice: ethPrice,
      balances: getBalanceReturnInfos
    }

    await dispatch(WalletActions.ethBalancesUpdated(balancesAndPrice))

    // Update Token Balances
    if (!visibleTokens) {
      return
    }

    const getTokenPrices = await Promise.all(visibleTokens.map(async (token) => {
      const emptyPrice = {
        assetTimeframeChange: '0',
        fromAsset: token.symbol,
        price: '0',
        toAsset: 'usd'
      }
      const price = await assetRatioController.getPrice([token.symbol.toLowerCase()], ['usd'], selectedPortfolioTimeline)
      return price.success ? price.values[0] : emptyPrice
    }))

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
      prices: { success: true, values: getTokenPrices }
    }

    await dispatch(WalletActions.tokenBalancesUpdated(tokenBalancesAndPrices))
  }
}

export function refreshTokenPriceHistory (selectedPortfolioTimeline: AssetPriceTimeframe) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = await getAPIProxy()
    const { assetRatioController } = apiProxy

    const { wallet: { accounts } } = getState()

    const result = await Promise.all(accounts.map(async (account) => {
      return Promise.all(account.tokens.map(async (token) => {
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

export function refreshTransactionHistory () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = await getAPIProxy()
    const { ethTxController } = apiProxy

    const { wallet: { accounts } } = getState()

    const transactionListInfo = await Promise.all(accounts.map(async (account) => {
      const transactions = await ethTxController.getAllTransactionInfo(account.address)
      return {
        account: {
          id: account.id,
          address: account.address,
          name: account.name
        },
        transactions: transactions.transactionInfos
      }
    }))

    await dispatch(WalletActions.setTransactionList(transactionListInfo))
  }
}

export function refreshNetworkInfo () {
  return async (dispatch: Dispatch) => {
    const apiProxy = await getAPIProxy()
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
    const apiProxy = await getAPIProxy()
    const { keyringController, walletHandler } = apiProxy

    const walletInfo = await walletHandler.getWalletInfo()

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
    const apiProxy = await getAPIProxy()
    const { braveWalletService } = apiProxy

    const { wallet: { accounts, activeOrigin } } = getState()

    // Get a list of accounts with permissions of the active origin
    const getAllPermissions = await Promise.all(accounts.map(async (account) => {
      const result = await braveWalletService.hasEthereumPermission(activeOrigin, account.address)
      if (result.hasPermission) {
        return account
      } else {
        return
      }
    }))
    const accountsWithPermission: (WalletAccountType | undefined)[] = getAllPermissions.filter((account) => account !== undefined)
    dispatch(WalletActions.setSitePermissions({ accounts: accountsWithPermission }))
  }
}
