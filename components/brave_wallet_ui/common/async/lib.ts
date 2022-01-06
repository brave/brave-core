// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { formatBalance } from '../../utils/format-balances'
import {
  AccountTransactions,
  BraveWallet,
  WalletAccountType,
  AccountInfo,
  GetERCTokenInfoReturnInfo
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { GetNetworkInfo } from '../../utils/network-utils'
import { GetTokenParam, GetFlattenedAccountBalances } from '../../utils/api-utils'
import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import { GetAccountsHardwareOperationResult } from '../hardware_operations'
import { LedgerEthereumKeyring, TrezorKeyring } from '../hardware/interfaces'

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
    const keyring = getHardwareKeyring(opts.hardware)
    if (keyring instanceof LedgerEthereumKeyring || keyring instanceof TrezorKeyring) {
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
    const service = getAPIProxy().jsonRpcService
    const result = await service.getBalance(address)
    if (result.error === BraveWallet.ProviderError.kSuccess) {
      resolve(formatBalance(result.balance, 18))
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

export async function getERCTokenInfo (contractAddress: string): Promise<GetERCTokenInfoReturnInfo> {
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
  const { ercTokenRegistry } = getAPIProxy()
  return (await ercTokenRegistry.getBuyUrl(address, symbol, amount)).url
}

export async function getBuyAssets () {
  const { ercTokenRegistry } = getAPIProxy()
  return (await ercTokenRegistry.getBuyTokens()).tokens
}

export function refreshBalances (currentNetwork: BraveWallet.EthereumChain) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { wallet: { accounts } } = getState()

    const { braveWalletService, jsonRpcService } = apiProxy

    const visibleTokensInfo = await braveWalletService.getUserAssets(currentNetwork.chainId)

    // Selected Network's Native Asset
    const nativeAsset: BraveWallet.ERCToken = {
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

    const visibleTokens: BraveWallet.ERCToken[] = visibleTokensInfo.tokens.length === 0 ? [nativeAsset] : visibleTokensInfo.tokens
    await dispatch(WalletActions.setVisibleTokensInfo(visibleTokens))

    const getBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const balanceInfo = await jsonRpcService.getBalance(account.address)
      return balanceInfo
    }))
    const balancesAndPrice = {
      fiatPrice: '',
      balances: getBalanceReturnInfos
    }
    await dispatch(WalletActions.nativeAssetBalancesUpdated(balancesAndPrice))

    const getERCTokenBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      return Promise.all(visibleTokens.map(async (token) => {
        if (token.isErc721) {
          return jsonRpcService.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address)
        }
        return jsonRpcService.getERC20TokenBalance(token.contractAddress, account.address)
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
    const { wallet: { accounts, selectedPortfolioTimeline, selectedNetwork, userVisibleTokensInfo, defaultCurrencies } } = getState()
    const { assetRatioService } = apiProxy
    const defaultFiatCurrency = defaultCurrencies.fiat.toLowerCase()
    // Update ETH Balances
    const getNativeAssetPrice = await assetRatioService.getPrice([selectedNetwork.symbol.toLowerCase()], [defaultFiatCurrency], selectedPortfolioTimeline)
    const nativeAssetPrice = getNativeAssetPrice.success ? getNativeAssetPrice.values.find((i) => i.toAsset === defaultFiatCurrency)?.price ?? '' : ''
    const getBalanceReturnInfos = accounts.map((account) => {
      const balanceInfo = {
        error: BraveWallet.ProviderError.kSuccess,
        errorMessage: '',
        balance: account.balance
      }
      return balanceInfo
    })
    const balancesAndPrice = {
      fiatPrice: nativeAssetPrice,
      balances: getBalanceReturnInfos
    }

    await dispatch(WalletActions.nativeAssetBalancesUpdated(balancesAndPrice))

    // Update Token Balances
    if (!userVisibleTokensInfo) {
      return
    }

    const getTokenPrices = await Promise.all(GetFlattenedAccountBalances(accounts).map(async (token) => {
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

    const getERCTokenBalanceReturnInfos = accounts.map((account) => {
      return account.tokens.map((token) => {
        const balanceInfo = {
          error: BraveWallet.ProviderError.kSuccess,
          errorMessage: '',
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

export function refreshTokenPriceHistory (selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { assetRatioService } = apiProxy

    const { wallet: { accounts, defaultCurrencies, selectedNetwork } } = getState()

    // If a tokens balance is 0 we do not make an unnecessary api call for price history of that token
    const priceHistory = await Promise.all(GetFlattenedAccountBalances(accounts).filter((t) => !t.token.isErc721 && t.balance > 0).map(async (token) => {
      return {
        contractAddress: token.token.contractAddress,
        history: await assetRatioService.getPriceHistory(
          GetTokenParam(selectedNetwork, token.token), defaultCurrencies.fiat.toLowerCase(), selectedPortfolioTimeline
        )
      }
    }))

    const priceHistoryWithBalances = accounts.map((account) => {
      return (account.tokens.filter((t) => !t.asset.isErc721).map((token) => {
        return {
          token: token,
          history: priceHistory.find((t) => token.asset.contractAddress === t.contractAddress)?.history ?? { success: true, values: [] }
        }
      }))
    })

    dispatch(WalletActions.portfolioPriceHistoryUpdated(priceHistoryWithBalances))
  }
}

export function refreshTransactionHistory (address?: string) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { ethTxService } = apiProxy

    const { wallet: { accounts, transactions } } = getState()

    const accountsToUpdate = address !== undefined
      ? accounts.filter(account => account.address === address)
      : accounts

    const freshTransactions: AccountTransactions = await accountsToUpdate.reduce(
      async (acc, account) => acc.then(async (obj) => {
        const { transactionInfos } = await ethTxService.getAllTransactionInfo(account.address)
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

    const networkList = await jsonRpcService.getAllNetworks()
    dispatch(WalletActions.setAllNetworks(networkList))
    const chainId = await jsonRpcService.getChainId()
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
    const getSelectedAccount = await keyringService.getSelectedAccount()
    const selectedAddress = getSelectedAccount.address

    // Fallback account address if selectedAccount returns null
    const fallbackAddress = walletInfo.accountInfos[0].address

    // If selectedAccount is null will setSelectedAccount to fallback address
    if (!selectedAddress) {
      await keyringService.setSelectedAccount(fallbackAddress)
      walletInfo.selectedAccount = fallbackAddress
    } else {
      // If a user has already created an wallet but then chooses to restore
      // a different wallet, getSelectedAccount still returns the previous wallets
      // selected account.
      // This check looks to see if the returned selectedAccount exist in the accountInfos
      // payload, if not it will setSelectedAccount to the fallback address
      if (!walletInfo.accountInfos.find((account) => account.address.toLowerCase() === selectedAddress?.toLowerCase())) {
        walletInfo.selectedAccount = fallbackAddress
        await keyringService.setSelectedAccount(fallbackAddress)
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
