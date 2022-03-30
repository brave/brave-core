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
  SupportedTestNetworks,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { getNetworkInfo, getNetworksByCoinType, getTokensCoinType } from '../../utils/network-utils'
import { getTokenParam, getFlattenedAccountBalances } from '../../utils/api-utils'
import Amount from '../../utils/amount'

import getAPIProxy from './bridge'
import { Dispatch, State, Store } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import { GetAccountsHardwareOperationResult } from '../hardware/types'
import LedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import FilecoinLedgerKeyring from '../hardware/ledgerjs/filecoin_ledger_keyring'
import { AllNetworksOption } from '../../options/network-filter-options'

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

export const getBalance = (address: string, coin: BraveWallet.CoinType): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const { jsonRpcService } = getAPIProxy()
    const chainId = await jsonRpcService.getChainId(coin)
    const result = await jsonRpcService.getBalance(address, coin, chainId.chainId)
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
  const { url, error } = await blockchainRegistry.getBuyUrl(
    BraveWallet.OnRampProvider.kWyre,
    BraveWallet.MAINNET_CHAIN_ID,
    address,
    symbol,
    amount
  )

  if (error) {
    console.log(`Failed to get buy URL: ${error}`)
  }

  return url
}

export async function getBuyAssets () {
  const { blockchainRegistry } = getAPIProxy()
  return (await blockchainRegistry.getBuyTokens(
    BraveWallet.OnRampProvider.kWyre,
    BraveWallet.MAINNET_CHAIN_ID)).tokens
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
        visible: false,
        tokenId: '',
        coingeckoId: '',
        chainId: network.chainId,
        coin: network.coin
      }

      // Get a list of user tokens for each coinType and network.
      const getTokenList = await braveWalletService.getUserAssets(network.chainId, network.coin)

      // Adds a logo and chainId to each token object
      const tokenList = getTokenList.tokens.map((token) => ({
        ...token,
        logo: `chrome://erc-token-images/${token.logo}`
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
        // Get CoinType SOL balances
        if (network.coin === BraveWallet.CoinType.SOL) {
          const getSolBalanceInfo = await jsonRpcService.getSolanaBalance(account.address, network.chainId)
          const solBalanceInfo = {
            ...getSolBalanceInfo,
            balance: getSolBalanceInfo.balance.toString(),
            chainId: network.chainId
          }
          return network.chainId === BraveWallet.LOCALHOST_CHAIN_ID &&
            getSolBalanceInfo.error !== 0
            ? { ...emptyBalance, chainId: network.chainId }
            : solBalanceInfo
        }

        // Get CoinType FIL balances
        if (account.coin === BraveWallet.CoinType.FIL) {
          if (defaultNetworks.some(n => n.chainId === network.chainId)) {
            // Get CoinType FIL balances
            if (network.coin === BraveWallet.CoinType.FIL) {
              const balanceInfo = await jsonRpcService.getBalance(account.address, account.coin, network.chainId)
              return {
                ...balanceInfo,
                chainId: network.chainId
              }
            }
          }

          return {
            ...emptyBalance,
            chainId: network.chainId
          }
        }

        // LOCALHOST will return an error until a local instance is
        // detected, we now will will return a 0 balance until it's detected.
        if (network.chainId === BraveWallet.LOCALHOST_CHAIN_ID && network.coin !== BraveWallet.CoinType.SOL) {
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
      const networks = getNetworksByCoinType(networkList, account.coin)
      if (account.coin === BraveWallet.CoinType.ETH) {
        return Promise.all(visibleTokens.map(async (token) => {
          if (networks.some(n => n.chainId === token.chainId)) {
            if (token.isErc721) {
              return jsonRpcService.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address, token?.chainId ?? '')
            }
            return jsonRpcService.getERC20TokenBalance(token.contractAddress, account.address, token?.chainId ?? '')
          }
          return emptyBalance
        }))
      } else if (account.coin === BraveWallet.CoinType.SOL) {
        return Promise.all(visibleTokens.map(async (token) => {
          if (networks.some(n => n.chainId === token.chainId)) {
            const getSolTokenBalance = await jsonRpcService.getSPLTokenAccountBalance(account.address, token.contractAddress, token.chainId)
            return {
              balance: getSolTokenBalance.amount,
              error: getSolTokenBalance.error,
              errorMessage: getSolTokenBalance.errorMessage
            }
          }
          return emptyBalance
        }))
      } else {
        // MULTICHAIN: We do not yet support getting
        // token balances for FIL
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
      const price = token.balance > 0 && !token.token.isErc721
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

    const { wallet: { accounts, defaultCurrencies, userVisibleTokensInfo, selectedNetworkFilter, networkList } } = getState()

    // By default we do not fetch Price history for Test Networks Tokens if
    // Selected Network Filter is all
    const filteredTokenInfo = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? userVisibleTokensInfo.filter((token) => !SupportedTestNetworks.includes(token.chainId))
      // If chainId is Localhost we also do a check for coinType to only
      // fetch Price History for for the correct tokens
      : selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID
        ? userVisibleTokensInfo.filter((token) =>
          token.chainId === selectedNetworkFilter.chainId &&
          getTokensCoinType(networkList, token) === selectedNetworkFilter.coin)
        // Fetch Price History for Tokens by Selected Network Filter's chainId
        : userVisibleTokensInfo.filter((token) => token.chainId === selectedNetworkFilter.chainId)

    // Get all Price History
    const priceHistory = await Promise.all(getFlattenedAccountBalances(accounts, filteredTokenInfo)
      // If a tokens balance is 0 we do not make an unnecessary api call for price history of that token
      .filter(({ token, balance }) => !token.isErc721 && balance > 0)
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
      return filteredTokenInfo
        .filter((token) => !token.isErc721)
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
    console.log(accounts, " <-> ", transactions)
    const accountsToUpdate = address !== undefined
      ? accounts.filter(account => account.address === address)
      : accounts

    const freshTransactions: AccountTransactions = await accountsToUpdate.reduce(
      async (acc, account) => acc.then(async (obj) => {
        const { transactionInfos } = await txService.getAllTransactionInfo(account.coin, account.address)
        console.log(account.address, "transactionInfos:", transactionInfos);
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

    // Get default accounts for each CoinType
    const defaultAccounts = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
      const defaultAccount = await keyringService.getSelectedAccount(coin)
      const defaultAccountAddress = defaultAccount.address
      return walletInfo.accountInfos.find((account) => account.address.toLowerCase() === defaultAccountAddress?.toLowerCase()) ?? {} as BraveWallet.AccountInfo
    }))
    const filteredDefaultAccounts = defaultAccounts.filter((account) => Object.keys(account).length !== 0)
    dispatch(WalletActions.setDefaultAccounts(filteredDefaultAccounts))

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
      const result = await braveWalletService.hasEthereumPermission(activeOrigin.origin, account.address)
      if (result.hasPermission) {
        return account
      }

      return undefined
    }))
    const accountsWithPermission: WalletAccountType[] = getAllPermissions.filter((account): account is WalletAccountType => account !== undefined)
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

export async function sendEthTransaction (store: Store, payload: SendEthTransactionParams) {
  const apiProxy = getAPIProxy()
  /***
   * Determine whether to create a legacy or EIP-1559 transaction.
   *
   * isEIP1559 is true IFF:
   *   - network supports EIP-1559
   *   - keyring supports EIP-1559 (ex: certain hardware wallets vendors)
   *   - payload: SendEthTransactionParams has specified EIP-1559 gas-pricing
   *     fields.
   *
   * In all other cases, fallback to legacy gas-pricing fields.
   */
  let isEIP1559
  switch (true) {
    // Transaction payload has hardcoded EIP-1559 gas fields.
    case payload.maxPriorityFeePerGas !== undefined && payload.maxFeePerGas !== undefined:
      isEIP1559 = true
      break

    // Transaction payload has hardcoded legacy gas fields.
    case payload.gasPrice !== undefined:
      isEIP1559 = false
      break

    // Check if network and keyring support EIP-1559.
    default:
      const { selectedAccount, selectedNetwork } = store.getState().wallet
      isEIP1559 = hasEIP1559Support(selectedAccount, selectedNetwork)
  }

  const { chainId } = await apiProxy.jsonRpcService.getChainId(BraveWallet.CoinType.ETH)

  let addResult
  const txData: BraveWallet.TxData = {
    nonce: '',
    // Estimated by eth_tx_service if value is '' for legacy transactions
    gasPrice: isEIP1559 ? '' : payload.gasPrice || '',
    // Estimated by eth_tx_service if value is ''
    gasLimit: payload.gas || '',
    to: payload.to,
    value: payload.value,
    data: payload.data || []
  }

  if (isEIP1559) {
    const txData1559: BraveWallet.TxData1559 = {
      baseData: txData,
      chainId,
      // Estimated by eth_tx_service if value is ''
      maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
      // Estimated by eth_tx_service if value is ''
      maxFeePerGas: payload.maxFeePerGas || '',
      gasEstimation: undefined
    }
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    const txDataUnion: BraveWallet.TxDataUnion = { ethTxData1559: txData1559 }
    addResult = await apiProxy.txService.addUnapprovedTransaction(txDataUnion, payload.from, null)
  } else {
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    const txDataUnion: BraveWallet.TxDataUnion = { ethTxData: txData }
    addResult = await apiProxy.txService.addUnapprovedTransaction(txDataUnion, payload.from, null)
  }
  return addResult
}

export async function sendFilTransaction (payload: SendFilTransactionParams) {
  const apiProxy = getAPIProxy()
  const filTxData: BraveWallet.FilTxData = {
    nonce: payload.nonce || '',
    gasPremium: payload.gasPremium || '',
    gasFeeCap: payload.gasFeeCap || '',
    gasLimit: payload.gasLimit || '',
    maxFee: payload.maxFee || '0',
    to: payload.to,
    from: payload.from,
    value: payload.value
  }
  // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
  return await apiProxy.txService.addUnapprovedTransaction({ filTxData: filTxData }, payload.from)
}

export async function sendSolTransaction (payload: SendSolTransactionParams) {
  const { solanaTxManagerProxy, txService } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeSystemProgramTransferTxData(payload.from, payload.to, BigInt(payload.value))
  // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
  return await txService.addUnapprovedTransaction({ solanaTxData: value.txData }, payload.from)
}

export async function sendSPLTransaction (payload: BraveWallet.SolanaTxData) {
  const { txService } = getAPIProxy()
  // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
  return await txService.addUnapprovedTransaction({ solanaTxData: payload }, payload.feePayer)
}
