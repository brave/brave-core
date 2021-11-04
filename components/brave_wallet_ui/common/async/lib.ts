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
  TokenInfo,
  WalletAccountType,
  SignHardwareTransactionType,
  TransactionInfo,
  kLedgerHardwareVendor,
  kTrezorHardwareVendor,
  APIProxyControllers,
  SignHardwareMessageOperationResult
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { GetNetworkInfo } from '../../utils/network-utils'
import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getLocale } from '../../../common/locale'

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
    if (account.address === address) {
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

    // Selected Network's Native Asset
    const nativeAsset: TokenInfo = {
      contractAddress: '',
      decimals: currentNetwork.decimals,
      isErc20: false,
      isErc721: false,
      logo: currentNetwork.iconUrls[0] ?? '',
      name: currentNetwork.symbolName,
      symbol: currentNetwork.symbol,
      visible: false
    }

    const visibleTokens: TokenInfo[] = visibleTokensInfo.tokens.length === 0 ? [nativeAsset] : visibleTokensInfo.tokens
    await dispatch(WalletActions.setVisibleTokensInfo(visibleTokens))

    // Update ETH Balances
    const getNativeAssetPrice = await assetRatioController.getPrice([nativeAsset.symbol.toLowerCase()], ['usd'], selectedPortfolioTimeline)
    const nativeAssetPrice = getNativeAssetPrice.success ? getNativeAssetPrice.values.find((i) => i.toAsset === 'usd')?.price ?? '0' : '0'
    const getBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const balanceInfo = await ethJsonRpcController.getBalance(account.address)
      return balanceInfo
    }))
    const balancesAndPrice = {
      usdPrice: nativeAssetPrice,
      balances: getBalanceReturnInfos
    }

    await dispatch(WalletActions.nativeAssetBalancesUpdated(balancesAndPrice))

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
    const apiProxy = await getAPIProxy()
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

export async function signTrezorTransaction (apiProxy: APIProxyControllers, path: string, txInfo: TransactionInfo): Promise<SignHardwareTransactionType> {
  const chainId = await apiProxy.ethJsonRpcController.getChainId()
  const approved = await apiProxy.ethTxController.approveHardwareTransaction(txInfo.id)
  if (!approved.success) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const transaction = await apiProxy.ethTxController.getTransactionInfo(txInfo.id)
  if (!transaction) {
    return { success: false, error: getLocale('braveWalletTransactionNotFoundSignError') }
  }
  txInfo.txData.baseData.nonce = transaction.info.txData.baseData.nonce
  const deviceKeyring = await apiProxy.getKeyringsByType(kTrezorHardwareVendor)
  const signed = await deviceKeyring.signTransaction(path, txInfo, chainId.chainId)
  if (!signed || !signed.success) {
    return { success: false, error: getLocale('braveWalletSignOnDeviceError') }
  }
  const { v, r, s } = signed.payload
  const result =
    await apiProxy.ethTxController.processHardwareSignature(txInfo.id, v, r.replace('0x', ''), s.replace('0x', ''))
  if (!result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerTransaction (apiProxy: APIProxyControllers, path: string, txInfo: TransactionInfo): Promise<SignHardwareTransactionType> {
  const approved = await apiProxy.ethTxController.approveHardwareTransaction(txInfo.id)
  if (!approved.success) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const data = await apiProxy.ethTxController.getTransactionMessageToSign(txInfo.id)
  if (!data) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }
  const deviceKeyring = await apiProxy.getKeyringsByType(kLedgerHardwareVendor)
  const signed = await deviceKeyring.signTransaction(path, data.message.replace('0x', ''))
  if (!signed) {
    return { success: false, error: getLocale('braveWalletSignOnDeviceError') }
  }
  const { v, r, s } = signed
  const result = await apiProxy.ethTxController.processHardwareSignature(txInfo.id, '0x' + v, r, s)
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signMessageWithHardwareKeyring (apiProxy: APIProxyControllers, vendor: string, path: string, address: string, message: string): Promise<SignHardwareMessageOperationResult> {
  const deviceKeyring = await apiProxy.getKeyringsByType(vendor)
  if (vendor === kLedgerHardwareVendor) {
    return deviceKeyring.signPersonalMessage(path, address, message)
  }

  return deviceKeyring.signPersonalMessage(path, message)
}
