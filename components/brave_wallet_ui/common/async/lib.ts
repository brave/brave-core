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
  GetBlockchainTokenInfoReturnInfo,
  SupportedCoinTypes,
  SupportedTestNetworks
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { getNetworkInfo, getNetworksByCoinType } from '../../utils/network-utils'
import { getTokenParam, getFlattenedAccountBalances } from '../../utils/api-utils'
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
  return async (dispatch: Dispatch, getState: () => State) => {
    const { braveWalletService } = getAPIProxy()
    const { wallet: { networkList } } = getState()

    const getVisibleAssets = await Promise.all(networkList.map(async (network) => {
      // Creates a network's Native Asset if not returned
      const nativeAsset: BraveWallet.BlockchainToken = {
        contractAddress: '',
        decimals: network.decimals,
        isErc20: false,
        isErc721: false,
        logo: network.iconUrls[0] ?? '',
        name: network.symbolName,
        symbol: network.symbol,
        // MULTICHAIN: Change visible back to false once getUserAssets returns
        // SOL and FIL by default.
        visible: network.coin === BraveWallet.CoinType.SOL || network.coin === BraveWallet.CoinType.FIL,
        tokenId: '',
        coingeckoId: '',
        chainId: network.chainId
      }

      // Get a list of user tokens for each coinType and network.
      const getTokenList = network.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
        network.coin === BraveWallet.CoinType.SOL || network.coin === BraveWallet.CoinType.FIL
        // Since LOCALHOST's chainId is shared between coinType networks,
        // this check will make sure we create the correct Native Asset for
        // that network.
        ? { tokens: [nativeAsset] } // MULTICHAIN: We do not yet support getting userAssets for FIL and SOL
        // Will be implemented here https://github.com/brave/brave-browser/issues/21547
        : await braveWalletService.getUserAssets(network.chainId)

      // Adds a logo and chainId to each token object
      const tokenList = getTokenList.tokens.map((token) => ({
        ...token,
        logo: token.symbol.toLowerCase() === 'sol' ? '' : `chrome://erc-token-images/${token.logo}`
      })) as BraveWallet.BlockchainToken[]
      return tokenList.length === 0 ? [nativeAsset] : tokenList
    }))
    const visibleAssets = getVisibleAssets.flat(1)
    await dispatch(WalletActions.setVisibleTokensInfo(visibleAssets))
  }
}

export function refreshBalances () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { jsonRpcService } = getAPIProxy()
    const { wallet: { accounts, userVisibleTokensInfo, networkList, defaultNetworks } } = getState()

    const emptyBalance = {
      balance: '0x0',
      error: 0,
      errorMessage: ''
    }

    const getNativeAssetsBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      const networks = getNetworksByCoinType(networkList, account.coin)

      return Promise.all(networks.map(async (network) => {
        // MULTICHAIN: Backend needs to allow chainId for getSolanaBalance method
        // Will be implemented here https://github.com/brave/brave-browser/issues/21695
        if (account.coin === BraveWallet.CoinType.SOL || account.coin === BraveWallet.CoinType.FIL) {
          if (defaultNetworks.some(n => n.chainId === network.chainId)) {
            // Get CoinType SOL balances
            if (network.coin === BraveWallet.CoinType.SOL) {
              const solBalanceInfo = await jsonRpcService.getSolanaBalance(account.address)
              return {
                ...solBalanceInfo,
                balance: solBalanceInfo.balance.toString(),
                chainId: network.chainId
              }
            }

            // Get CoinType FIL balances
            if (network.coin === BraveWallet.CoinType.FIL) {
              const balanceInfo = await jsonRpcService.getBalance(account.address, account.coin, network.chainId)
              return {
                ...balanceInfo,
                chainId: network.chainId
              }
            }
          }

          // Return emptyBalance for CoinType FIL or SOL if network
          // is not included in defaultNetworks. Will update when this
          // is implemented https://github.com/brave/brave-browser/issues/21695
          return {
            ...emptyBalance,
            chainId: network.chainId
          }
        }

        // LOCALHOST will return an error until a local instance is
        // detected, we now will will return a 0 balance until it's detected.
        if (network.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
          const localhostBalanceInfo = await jsonRpcService.getBalance(account.address, account.coin, network.chainId)
          const info = localhostBalanceInfo.error === 0 ? localhostBalanceInfo : emptyBalance
          return {
            ...info,
            chainId: network.chainId
          }
        }

        // Get CoinType ETH balances
        const balanceInfo = await jsonRpcService.getBalance(account.address, account.coin, network.chainId)
        return {
          ...balanceInfo,
          chainId: network.chainId
        }
      }))
    }))

    await dispatch(WalletActions.nativeAssetBalancesUpdated({
      balances: getNativeAssetsBalanceReturnInfos
    }))

    const visibleTokens = userVisibleTokensInfo.filter(asset => asset.contractAddress !== '')

    const getBlockchainTokensBalanceReturnInfos = await Promise.all(accounts.map(async (account) => {
      if (account.coin === BraveWallet.CoinType.ETH) {
        return Promise.all(visibleTokens.map(async (token) => {
          if (token.isErc721) {
            return jsonRpcService.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address, token?.chainId ?? '')
          }
          return jsonRpcService.getERC20TokenBalance(token.contractAddress, account.address, token?.chainId ?? '')
        }))
      } else {
        // MULTICHAIN: We do not yet support getting
        // token balances for SOL and FIL
        // Will be implemented here https://github.com/brave/brave-browser/issues/21695
        return []
      }
    }))

    await dispatch(WalletActions.tokenBalancesUpdated({
      balances: getBlockchainTokensBalanceReturnInfos
    }))
  }
}

export function refreshPrices () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { assetRatioService } = getAPIProxy()
    const { wallet: { accounts, selectedPortfolioTimeline, userVisibleTokensInfo, defaultCurrencies, networkList } } = getState()
    const defaultFiatCurrency = defaultCurrencies.fiat.toLowerCase()

    // Return if userVisibleTokensInfo is empty
    if (!userVisibleTokensInfo) {
      return
    }

    // Get prices for each networks native asset
    const mainnetList = networkList.filter((network) => !SupportedTestNetworks.includes(network.chainId))
    const getNativeAssetPrices = await Promise.all(mainnetList.map(async (network) => {
      const getNativeAssetPrice = await assetRatioService.getPrice([network.symbol.toLowerCase()], [defaultFiatCurrency], selectedPortfolioTimeline)
      const nativeAssetPrice = getNativeAssetPrice.success ? getNativeAssetPrice.values.find((i) => i.toAsset === defaultFiatCurrency)?.price ?? '' : ''
      return {
        fromAsset: network.symbol.toLowerCase(),
        toAsset: defaultFiatCurrency,
        price: nativeAssetPrice,
        assetTimeframeChange: ''
      }
    }))

    // Get prices for all other tokens on each network
    const blockChainTokenInfo = userVisibleTokensInfo.filter((token) => token.contractAddress !== '')
    const getTokenPrices = await Promise.all(getFlattenedAccountBalances(accounts, blockChainTokenInfo).map(async (token) => {
      const emptyPrice = {
        fromAsset: token.token.symbol,
        toAsset: defaultFiatCurrency,
        price: '',
        assetTimeframeChange: ''
      }

      // If a tokens balance is 0 we do not make an unnecessary api call for the price of that token
      const price = token.balance > 0 && token.token.isErc20
        ? await assetRatioService.getPrice([getTokenParam(token.token)], [defaultFiatCurrency], selectedPortfolioTimeline)
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
        ...getNativeAssetPrices,
        ...getTokenPrices
      ]
    }))
  }
}

export function refreshTokenPriceHistory (selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { assetRatioService } = apiProxy

    const { wallet: { accounts, defaultCurrencies, userVisibleTokensInfo } } = getState()

    // Get all Price History
    const priceHistory = await Promise.all(getFlattenedAccountBalances(accounts, userVisibleTokensInfo)
      // If a tokens balance is 0 we do not make an unnecessary api call for price history of that token
      // Will remove testnetwork filter when this is implemented
      // https://github.com/brave/brave-browser/issues/20780
      .filter(({ token, balance }) => !token.isErc721 && balance > 0 && !SupportedTestNetworks.includes(token.chainId))
      .map(async ({ token }) => ({
        // If a visible asset has a contractAddress of ''
        // it is a native asset so we use a symbol instead.
        contractAddress: token.contractAddress ? token.contractAddress : token.symbol,
        history: await assetRatioService.getPriceHistory(
          getTokenParam(token), defaultCurrencies.fiat.toLowerCase(), selectedPortfolioTimeline
        )
      }))
    )

    // Combine Price History and Balances
    const priceHistoryWithBalances = accounts.map((account) => {
      return userVisibleTokensInfo
        // Will remove testnetwork filter when this is implemented
        // https://github.com/brave/brave-browser/issues/20780
        .filter((token) => !token.isErc721 && !SupportedTestNetworks.includes(token.chainId))
        .map((token) => {
          const balance = token.contractAddress
            ? account.tokenBalanceRegistry[token.contractAddress.toLowerCase()]
            : account.nativeBalanceRegistry[token.chainId || '']
          const contractAddress = token.contractAddress ? token.contractAddress : token.symbol
          return {
            token,
            balance: balance || '0',
            history: priceHistory.find((t) => contractAddress === t.contractAddress)?.history ?? { success: true, values: [] }
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
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { jsonRpcService } = apiProxy
    const { wallet: { selectedCoin, isFilecoinEnabled, isSolanaEnabled, isTestNetworksEnabled } } = getState()

    // Get All Networks
    const getFullNetworkList = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
      // MULTICHAIN: While we are still in development for FIL and SOL,
      // we will not use their networks unless enabled by brave://flags
      if (coin === BraveWallet.CoinType.FIL && !isFilecoinEnabled) {
        return []
      }
      if (coin === BraveWallet.CoinType.SOL && !isSolanaEnabled) {
        return []
      }
      const networkList = await jsonRpcService.getAllNetworks(coin)
      return networkList.networks
    }))
    const flattenedNetworkList = getFullNetworkList.flat(1)
    const networkList =
      isTestNetworksEnabled
        ? flattenedNetworkList
        : flattenedNetworkList.filter((network) => !SupportedTestNetworks.includes(network.chainId))
    dispatch(WalletActions.setAllNetworks(networkList))

    // Get default network for each coinType
    const defaultNetworks = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
      const coinsChainId = await jsonRpcService.getChainId(coin)
      const network = getNetworkInfo(coinsChainId.chainId, networkList)
      return network
    }))
    dispatch(WalletActions.setDefaultNetworks(defaultNetworks))

    // Get current selected networks info
    const chainId = await jsonRpcService.getChainId(selectedCoin)
    const currentNetwork = getNetworkInfo(chainId.chainId, networkList)
    dispatch(WalletActions.setNetwork(currentNetwork))
    return currentNetwork
  }
}

export function refreshKeyringInfo () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { wallet: { selectedCoin } } = getState()
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
    const getSelectedAccount = await keyringService.getSelectedAccount(selectedCoin)
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
