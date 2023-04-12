// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assert } from 'chrome://resources/js/assert_ts.js'

import {
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  AccountTransactions,
  BraveWallet,
  BalancePayload,
  WalletAccountType,
  BraveKeyrings,
  GetBlockchainTokenInfoReturnInfo,
  SupportedCoinTypes,
  SupportedTestNetworks,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SolanaSerializedTransactionParams,
  SupportedOnRampNetworks,
  SupportedOffRampNetworks
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import {
  getFilecoinKeyringIdFromNetwork,
  getNetworksByCoinType
} from '../../utils/network-utils'
import { getTokenParam, getFlattenedAccountBalances } from '../../utils/api-utils'
import Amount from '../../utils/amount'
import { sortTransactionByDate } from '../../utils/tx-utils'
import { getBatTokensFromList, getNativeTokensFromList, getUniqueAssets } from '../../utils/asset-utils'
import { loadTimeData } from '../../../common/loadTimeData'
import { getVisibleNetworksList } from '../slices/api.slice'

import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import { GetAccountsHardwareOperationResult, SolDerivationPaths } from '../hardware/types'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import { AllNetworksOption, AllNetworksOptionDefault } from '../../options/network-filter-options'
import { AllAccountsOption } from '../../options/account-filter-options'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
import { deserializeOrigin, makeSerializableTransaction } from '../../utils/model-serialization-utils'
import { WalletPageActions } from '../../page/actions'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'
import { IPFS_PROTOCOL, isIpfs, stripERC20TokenImageURL } from '../../utils/string-utils'

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
      reject(result.errorMessage)
    }
  })
}

export const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Promise<BraveWallet.HardwareWalletAccount[]> => {
  return new Promise(async (resolve, reject) => {
    const keyring = getHardwareKeyring(opts.hardware, opts.coin, opts.onAuthorized)
    if ((keyring instanceof EthereumLedgerBridgeKeyring || keyring instanceof TrezorBridgeKeyring) && opts.scheme) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.scheme)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (keyring instanceof FilecoinLedgerBridgeKeyring && opts.network) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.network)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (keyring instanceof SolanaLedgerBridgeKeyring && opts.network && opts.scheme) {
      keyring.getAccounts(opts.startIndex, opts.stopIndex, opts.scheme as SolDerivationPaths)
        .then(async (result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            const { braveWalletService } = getAPIProxy()
            const addressesEncoded = await braveWalletService.base58Encode(
              result.payload.map((hardwareAccount) => [...(hardwareAccount.addressBytes || [])])
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

export const getBalance = async (address: string, coin: BraveWallet.CoinType): Promise<string> => {
  const { jsonRpcService } = getAPIProxy()
  const chainId = await jsonRpcService.getChainId(coin)
  return await getBalanceForChainId(address, coin, chainId.chainId)
}

export function getBalanceForChainId (address: string, coin: BraveWallet.CoinType, chainId: string): Promise<string> {
  return new Promise(async (resolve, reject) => {
    const { jsonRpcService } = getAPIProxy()
    if (coin === BraveWallet.CoinType.SOL) {
      const result = await jsonRpcService.getSolanaBalance(address, chainId)
      if (result.error === BraveWallet.SolanaProviderError.kSuccess) {
        resolve(Amount.normalize(result.balance.toString()))
      } else {
        reject(result.errorMessage)
      }
      return
    }
    const result = await jsonRpcService.getBalance(address, coin, chainId)
    if (result.error === BraveWallet.ProviderError.kSuccess) {
      resolve(Amount.normalize(result.balance))
    } else {
      reject(result.errorMessage)
    }
  })
}

export function getTokenBalanceForChainId (contract: string, address: string, coin: BraveWallet.CoinType, chainId: string): Promise<string> {
  return new Promise(async (resolve, reject) => {
    const { jsonRpcService } = getAPIProxy()
    if (coin === BraveWallet.CoinType.SOL) {
      const result = await jsonRpcService.getSPLTokenAccountBalance(address, contract, chainId)
      if (result.error === BraveWallet.SolanaProviderError.kSuccess) {
        resolve(Amount.normalize(result.amount))
      } else {
        reject(result.errorMessage)
      }
      return
    }
    const result = await jsonRpcService.getERC20TokenBalance(contract, address, chainId)
    if (result.error === BraveWallet.ProviderError.kSuccess) {
      resolve(Amount.normalize(result.balance))
    } else {
      reject(result.errorMessage)
    }
  })
}

export async function getChecksumEthAddress (value: string) {
  const { keyringService } = getAPIProxy()
  return (await keyringService.getChecksumEthAddress(value))
}

export async function isBase58EncodedSolanaPubkey (value: string) {
  const { braveWalletService } = getAPIProxy()
  return braveWalletService.isBase58EncodedSolanaPubkey(value)
}

export async function isStrongPassword (value: string) {
  const apiProxy = getAPIProxy()
  return (await apiProxy.keyringService.isStrongPassword(value)).result
}

export async function enableEnsOffchainLookup () {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.setEnsOffchainLookupResolveMethod(BraveWallet.ResolveMethod.kEnabled)
}

export async function findENSAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.ensGetEthAddr(address)
}

export async function findSNSAddress (address: string) {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.snsGetSolAddr(address)
}

export async function findUnstoppableDomainAddress (address: string, token: BraveWallet.BlockchainToken | null) {
  const apiProxy = getAPIProxy()
  return apiProxy.jsonRpcService.unstoppableDomainsGetWalletAddr(address, token)
}

export async function getBlockchainTokenInfo (contractAddress: string): Promise<GetBlockchainTokenInfoReturnInfo> {
  const apiProxy = getAPIProxy()
  return (await apiProxy.assetRatioService.getTokenInfo(contractAddress))
}

export async function findHardwareAccountInfo (
  address: string
): Promise<BraveWallet.AccountInfo | false> {
  const apiProxy = getAPIProxy()
  const result = (await apiProxy.walletHandler.getWalletInfo()).walletInfo
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

export async function getBuyAssetUrl (args: {
  asset: BraveWallet.BlockchainToken
  onRampProvider: BraveWallet.OnRampProvider
  chainId: string
  address: string
  amount: string
  currencyCode: string
}) {
  const { assetRatioService } = getAPIProxy()
  const { url, error } = await assetRatioService.getBuyUrlV1(
    args.onRampProvider,
    args.chainId,
    args.address,
    args.asset.symbol,
    args.amount,
    args.currencyCode
  )

  if (error) {
    console.log(`Failed to get buy URL: ${error}`)
  }

  return url
}

export async function getSellAssetUrl (args: {
  asset: BraveWallet.BlockchainToken
  offRampProvider: BraveWallet.OffRampProvider
  chainId: string
  address: string
  amount: string
  currencyCode: string
}) {
  const { assetRatioService } = getAPIProxy()
  const { url, error } = await assetRatioService.getSellUrl(
    args.offRampProvider,
    args.chainId,
    args.address,
    args.asset.symbol,
    args.amount,
    args.currencyCode
  )

  if (error) {
    console.log(`Failed to get sell URL: ${error}`)
  }

  return url
}

export const getTokenList = async (
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
): Promise<{ tokens: BraveWallet.BlockchainToken[] }> => {
  const { blockchainRegistry } = getAPIProxy()
  return blockchainRegistry.getAllTokens(network.chainId, network.coin)
}

export async function getBuyAssets (onRampProvider: BraveWallet.OnRampProvider, chainId: string) {
  const { blockchainRegistry } = getAPIProxy()
  return (await blockchainRegistry.getBuyTokens(
    onRampProvider,
    chainId)).tokens
}

export const getAllBuyAssets = async (): Promise<{
  rampAssetOptions: BraveWallet.BlockchainToken[]
  sardineAssetOptions: BraveWallet.BlockchainToken[]
  transakAssetOptions: BraveWallet.BlockchainToken[]
  allAssetOptions: BraveWallet.BlockchainToken[]
}> => {
  const { blockchainRegistry } = getAPIProxy()
  const { kRamp, kSardine, kTransak } = BraveWallet.OnRampProvider

  const rampAssetsPromises = await Promise.all(
    SupportedOnRampNetworks.map(chainId => blockchainRegistry.getBuyTokens(kRamp, chainId))
  )
  const sardineAssetsPromises = await Promise.all(
    SupportedOnRampNetworks.map(chainId => blockchainRegistry.getBuyTokens(kSardine, chainId))
  )

  const transakAssetsPromises = await Promise.all(
    SupportedOnRampNetworks.map(chainId => blockchainRegistry.getBuyTokens(kTransak, chainId))
  )

  // add token logos
  const rampAssetOptions: BraveWallet.BlockchainToken[] =
    await Promise.all(rampAssetsPromises
      .flatMap(p => p.tokens)
      .map(await addLogoToToken))

  const sardineAssetOptions: BraveWallet.BlockchainToken[] =
    await Promise.all(sardineAssetsPromises
      .flatMap(p => p.tokens)
      .map(await addLogoToToken))

  const transakAssetOptions: BraveWallet.BlockchainToken[] =
    await Promise.all(transakAssetsPromises
      .flatMap(p => p.tokens)
      .map(await addLogoToToken))

  // separate native assets from tokens
  const {
    tokens: rampTokenOptions,
    nativeAssets: rampNativeAssetOptions
  } = getNativeTokensFromList(rampAssetOptions)

  const {
    tokens: sardineTokenOptions,
    nativeAssets: sardineNativeAssetOptions
  } = getNativeTokensFromList(sardineAssetOptions)

  const {
    tokens: transakTokenOptions,
    nativeAssets: transakNativeAssetOptions
  } = getNativeTokensFromList(transakAssetOptions)

  // separate BAT from other tokens
  const {
    bat: rampBatTokens,
    nonBat: rampNonBatTokens
  } = getBatTokensFromList(rampTokenOptions)

  const {
    bat: sardineBatTokens,
    nonBat: sardineNonBatTokens
  } = getBatTokensFromList(sardineTokenOptions)

  const {
    bat: transakBatTokens,
    nonBat: transakNonBatTokens
  } = getBatTokensFromList(transakTokenOptions)

  // sort lists
  // Move Gas coins and BAT to front of list
  const sortedRampOptions = [...rampNativeAssetOptions, ...rampBatTokens, ...rampNonBatTokens]
  const sortedSardineOptions = [...sardineNativeAssetOptions, ...sardineBatTokens, ...sardineNonBatTokens]
  const sortedTransakOptions = [...transakNativeAssetOptions, ...transakBatTokens, ...transakNonBatTokens]

  const results = {
    rampAssetOptions: sortedRampOptions,
    sardineAssetOptions: sortedSardineOptions,
    transakAssetOptions: sortedTransakOptions,
    allAssetOptions: getUniqueAssets([
      ...sortedRampOptions,
      ...sortedSardineOptions,
      ...sortedTransakOptions
    ])
  }

  return results
}

export const getAllSellAssets = async (): Promise<{
  rampAssetOptions: BraveWallet.BlockchainToken[]
  allAssetOptions: BraveWallet.BlockchainToken[]
}> => {
  const { blockchainRegistry } = getAPIProxy()
  const { kRamp } = BraveWallet.OffRampProvider

  const rampAssetsPromises = await Promise.all(
    SupportedOffRampNetworks.map(chainId => blockchainRegistry.getSellTokens(kRamp, chainId))
  )

  // add token logos
  const rampAssetOptions: BraveWallet.BlockchainToken[] =
    await Promise.all(rampAssetsPromises
      .flatMap(p => p.tokens)
      .map(await addLogoToToken))

  // separate native assets from tokens
  const {
    tokens: rampTokenOptions,
    nativeAssets: rampNativeAssetOptions
  } = getNativeTokensFromList(rampAssetOptions)

  // separate BAT from other tokens
  const {
    bat: rampBatTokens,
    nonBat: rampNonBatTokens
  } = getBatTokensFromList(rampTokenOptions)

  // moves Gas coins and BAT to front of list
  const sortedRampOptions = [...rampNativeAssetOptions, ...rampBatTokens, ...rampNonBatTokens]

  const results = {
    rampAssetOptions: sortedRampOptions,
    allAssetOptions: getUniqueAssets([
      ...sortedRampOptions
    ])
  }

  return results
}

export function getKeyringIdFromCoin (coin: BraveWallet.CoinType): BraveKeyrings {
  if (coin === BraveWallet.CoinType.FIL) {
    return BraveWallet.FILECOIN_KEYRING_ID
  }
  if (coin === BraveWallet.CoinType.SOL) {
    return BraveWallet.SOLANA_KEYRING_ID
  }
  assert(coin === BraveWallet.CoinType.ETH, '')
  return BraveWallet.DEFAULT_KEYRING_ID
}

export async function getKeyringIdFromAddress (address: string): Promise<string> {
  const apiProxy = getAPIProxy()
  const result = (await apiProxy.walletHandler.getWalletInfo()).walletInfo
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

export async function hasJupiterFeesForMint (mint: string): Promise<boolean> {
  const { swapService } = getAPIProxy()
  return (await swapService.hasJupiterFeesForTokenMint(mint)).result
}

export function refreshVisibleTokenInfo (targetNetwork?: BraveWallet.NetworkInfo) {
  return async (dispatch: Dispatch, getState: () => State) => {
    const api = getAPIProxy()
    const { braveWalletService } = api
    const networkList = await getVisibleNetworksList(api)

    async function inner (network: BraveWallet.NetworkInfo) {
      // Creates a network's Native Asset if not returned
      const nativeAsset: BraveWallet.BlockchainToken = {
        contractAddress: '',
        decimals: network.decimals,
        isErc20: false,
        isErc721: false,
        isErc1155: false,
        isNft: false,
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
    }

    const visibleAssets = targetNetwork
      ? await inner(targetNetwork)
      : await Promise.all(networkList.map(async (item) => await inner(item)))

    const userVisibleTokensInfo = visibleAssets.flat(1)
    await dispatch(WalletActions.setVisibleTokensInfo(userVisibleTokensInfo))
    const nfts = userVisibleTokensInfo.filter((asset) => asset.isErc721 || asset.isNft)
    dispatch(WalletPageActions.getNftsPinningStatus(nfts))
  }
}

function reportActiveWalletsToP3A (accounts: WalletAccountType[],
                                   nativeBalances: BalancePayload[][],
                                   blockchainTokenBalances: BalancePayload[][]) {
  const { braveWalletP3A } = getAPIProxy()
  const coinsActiveAddresses: {
    [coin: BraveWallet.CoinType]: {
      [address: string]: boolean
    }
  } = {}
  const countTestNetworks = loadTimeData.getBoolean(BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY)
  for (const balances of [nativeBalances, blockchainTokenBalances]) {
    for (const [index, account] of accounts.entries()) {
      const balanceInfos = balances[index]

      const coinIndex = SupportedCoinTypes.indexOf(account.coin)
      if (coinIndex === -1) {
        continue
      }
      let coinActiveAddresses = coinsActiveAddresses[coinIndex]
      if (!coinActiveAddresses) {
        coinActiveAddresses = coinsActiveAddresses[coinIndex] = {}
      }

      for (const balanceInfo of balanceInfos) {
        // Skip counting empty balances and balances on testnets
        // Testnet balance counting can be enabled via the --p3a-count-wallet-test-networks switch
        if (!balanceInfo ||
          (!countTestNetworks &&
            SupportedTestNetworks.includes(balanceInfo.chainId)) ||
          balanceInfo.balance === '0x0' ||
          balanceInfo.balance === '0' ||
          balanceInfo.balance === '') {
          continue
        }

        coinActiveAddresses[account.address] = true
      }
    }
  }
  for (const [coin, coinActiveAddresses] of Object.entries(coinsActiveAddresses)) {
    braveWalletP3A.recordActiveWalletCount(
      Object.keys(coinActiveAddresses).length,
      SupportedCoinTypes[coin])
  }
}

export function refreshBalances () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const api = getAPIProxy()
    const { jsonRpcService } = api
    const {
      wallet: { accounts, userVisibleTokensInfo }
    } = getState()

    const networkList = await getVisibleNetworksList(api)

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
          if (networkList.some(n => n.chainId === network.chainId)) {
            // Get CoinType FIL balances
            if (network.coin === BraveWallet.CoinType.FIL && account.keyringId === getFilecoinKeyringIdFromNetwork(network)) {
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
          let balanceInfo = emptyBalance
          if (networks.some(n => n.chainId === token.chainId)) {
            if (token.isErc721) {
              balanceInfo =
                await jsonRpcService.getERC721TokenBalance(token.contractAddress, token.tokenId ?? '', account.address, token?.chainId ?? '')
            } else {
              balanceInfo =
               await jsonRpcService.getERC20TokenBalance(token.contractAddress, account.address, token?.chainId ?? '')
            }
          }
          return {
            ...balanceInfo,
            chainId: token?.chainId ?? ''
          }
        }))
      } else if (account.coin === BraveWallet.CoinType.SOL) {
        return Promise.all(visibleTokens.map(async (token) => {
          if (networks.some(n => n.chainId === token.chainId)) {
            const getSolTokenBalance = await jsonRpcService.getSPLTokenAccountBalance(account.address, token.contractAddress, token.chainId)
            return {
              balance: token.isNft ? getSolTokenBalance.uiAmountString : getSolTokenBalance.amount,
              error: getSolTokenBalance.error,
              errorMessage: getSolTokenBalance.errorMessage,
              chainId: token.chainId
            }
          }
          return {
            ...emptyBalance,
            chainId: token?.chainId ?? ''
          }
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

    reportActiveWalletsToP3A(
      accounts,
      getNativeAssetsBalanceReturnInfos,
      getBlockchainTokensBalanceReturnInfos
    )
  }
}

export function refreshPrices () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const api = getAPIProxy()
    const { assetRatioService } = api
    const {
      wallet: {
        accounts,
        selectedPortfolioTimeline,
        userVisibleTokensInfo,
        defaultCurrencies
      }
    } = getState()
    const networkList = await getVisibleNetworksList(api)

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
        assetTimeframeChange: '',
        contractAddress: '',
        chainId: network.chainId
      }
    }))

    // Get prices for all other tokens on each network
    const blockChainTokenInfo = userVisibleTokensInfo.filter((token) => token.contractAddress !== '')
    const getTokenPrices = await Promise.all(getFlattenedAccountBalances(accounts, blockChainTokenInfo).map(async (token) => {
      const emptyPrice = {
        fromAsset: token.token.symbol,
        toAsset: defaultFiatCurrency,
        price: '',
        assetTimeframeChange: '',
        contractAddress: token.token.contractAddress,
        chainId: token.token.chainId
      }

      // If a tokens balance is 0 we do not make an unnecessary api call for the price of that token

      const price = token.balance > 0 && !token.token.isErc721
        ? await assetRatioService.getPrice(
          [getTokenParam(token.token)],
          [defaultFiatCurrency],
          selectedPortfolioTimeline
        )
        : {
          values: [{ ...emptyPrice, price: '0' }],
          success: true
        }

      const tokenPrice = {
        ...price.values[0],
        fromAsset: token.token.symbol.toLowerCase(),
        contractAddress: token.token.contractAddress,
        chainId: token.token.chainId
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

    const { wallet: { accounts, defaultCurrencies, userVisibleTokensInfo, selectedNetworkFilter, selectedAccountFilter } } = getState()

    // By default, we do not fetch Price history for Test Networks Tokens if
    // Selected Network Filter is all
    const filteredTokenInfo = selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? userVisibleTokensInfo.filter((token) => !SupportedTestNetworks.includes(token.chainId))
      // If chainId is Localhost we also do a check for coinType to only
      // fetch Price History for the correct tokens
      : selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID
        ? userVisibleTokensInfo.filter((token) =>
          token.chainId === selectedNetworkFilter.chainId &&
          token.coin === selectedNetworkFilter.coin)
        // Fetch Price History for Tokens by Selected Network Filter's chainId
        : userVisibleTokensInfo.filter((token) => token.chainId === selectedNetworkFilter.chainId)

    const foundSelectedAccountForFilter = accounts.find(account => account.id === selectedAccountFilter)

    // If a selectedAccountFilter is selected, we only return the selectedAccountFilter
    // in the list.
    const accountsList = selectedAccountFilter === AllAccountsOption.id
      ? accounts
      : foundSelectedAccountForFilter ? [foundSelectedAccountForFilter] : []

    // Get all Price History
    const priceHistory = await Promise.all(getFlattenedAccountBalances(accountsList, filteredTokenInfo)
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
    const priceHistoryWithBalances = accountsList.map((account) => {
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

    const accountsToUpdate = address !== undefined
      ? accounts.filter(account => account.address === address)
      : accounts

    const freshTransactions: AccountTransactions = await accountsToUpdate.reduce<Promise<AccountTransactions>>(
      async (acc, account) => acc.then(async (obj) => {
        const { transactionInfos } = await txService.getAllTransactionInfo(account.coin, account.address)
        const serializedTransactionInfos = transactionInfos.map(makeSerializableTransaction)
        obj[account.address] = sortTransactionByDate(serializedTransactionInfos, 'descending')
        return obj
      }), Promise.resolve({}))

    dispatch(WalletActions.setAccountTransactions({
      ...transactions,
      ...freshTransactions
    }))
  }
}

export function refreshKeyringInfo () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { keyringService, walletHandler, jsonRpcService } = apiProxy
    const walletInfoBase = (await walletHandler.getWalletInfo()).walletInfo
    const walletInfo = { ...walletInfoBase, visibleTokens: [], selectedAccount: '' }

    // Get/Set selectedAccount
    if (!walletInfo.isWalletCreated) {
      dispatch(WalletActions.initialized(walletInfo))
      return
    }

    // Get default accounts for each CoinType
    const defaultAccounts = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
      const chainId = await jsonRpcService.getChainId(coin)
      const defaultAccount = coin === BraveWallet.CoinType.FIL
        ? await keyringService.getFilecoinSelectedAccount(chainId.chainId)
        : await keyringService.getSelectedAccount(coin)
      const defaultAccountAddress = defaultAccount.address
      return walletInfo.accountInfos.find((account) => account.address.toLowerCase() === defaultAccountAddress?.toLowerCase()) ?? {} as BraveWallet.AccountInfo
    }))
    const filteredDefaultAccounts = defaultAccounts.filter((account) => Object.keys(account).length !== 0)
    dispatch(WalletActions.setDefaultAccounts(filteredDefaultAccounts))

    const { coin: selectedCoin } =
      await apiProxy.braveWalletService.getSelectedCoin()

    let selectedAccount = { address: null } as { address: string | null }

    if (selectedCoin === BraveWallet.CoinType.FIL) {
      const coinsChainId = await jsonRpcService.getChainId(selectedCoin)
      selectedAccount = await keyringService.getFilecoinSelectedAccount(
        coinsChainId.chainId
      )
    }

    if (selectedCoin && selectedCoin !== BraveWallet.CoinType.FIL) {
      selectedAccount = await keyringService.getSelectedAccount(selectedCoin)
    }

    // Get selectedAccountAddress
    const selectedAddress = selectedAccount.address

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
      const result = await braveWalletService.hasPermission(account.coin, deserializeOrigin(activeOrigin.origin), account.address)
      if (result.success && result.hasPermission) {
        return account
      }

      return undefined
    }))
    const accountsWithPermission: WalletAccountType[] = getAllPermissions.filter((account): account is WalletAccountType => account !== undefined)
    dispatch(WalletActions.setSitePermissions({ accounts: accountsWithPermission }))
  }
}

export async function sendEthTransaction (payload: SendEthTransactionParams) {
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
      isEIP1559 = payload.hasEIP1559Support
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
    data: payload.data || [],
    signOnly: false,
    signedTransaction: ''
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
    addResult = await apiProxy.txService.addUnapprovedTransaction(txDataUnion, payload.from, null, null)
  } else {
    // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
    const txDataUnion: BraveWallet.TxDataUnion = { ethTxData: txData }
    addResult = await apiProxy.txService.addUnapprovedTransaction(txDataUnion, payload.from, null, null)
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
  return await apiProxy.txService.addUnapprovedTransaction({ filTxData: filTxData }, payload.from, null, null)
}

export async function sendSolTransaction (payload: SendSolTransactionParams) {
  const { solanaTxManagerProxy, txService } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeSystemProgramTransferTxData(payload.from, payload.to, BigInt(payload.value))
  // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
  return await txService.addUnapprovedTransaction({ solanaTxData: value.txData }, payload.from, null, null)
}

export async function sendSPLTransaction (payload: BraveWallet.SolanaTxData) {
  const { txService } = getAPIProxy()
  // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
  return await txService.addUnapprovedTransaction({ solanaTxData: payload }, payload.feePayer, null, null)
}

export async function sendSolanaSerializedTransaction (payload: SolanaSerializedTransactionParams) {
  const { solanaTxManagerProxy, txService } = getAPIProxy()
  const result = await solanaTxManagerProxy.makeTxDataFromBase64EncodedTransaction(
    payload.encodedTransaction,
    payload.txType,
    payload.sendOptions || null
  )
  if (result.error !== BraveWallet.ProviderError.kSuccess) {
    console.error(`Failed to sign Solana message: ${result.errorMessage}`)
    return { success: false, errorMessage: result.errorMessage, txMetaId: '' }
  } else {
    return await txService.addUnapprovedTransaction(
      // @ts-expect-error google closure is ok with undefined for other fields but mojom runtime is not
      { solanaTxData: result.txData },
      payload.from,
      null,
      payload.groupId || null
    )
  }
}

export function getSwapService () {
  const { swapService } = getAPIProxy()
  return swapService
}

export function getEthTxManagerProxy () {
  const { ethTxManagerProxy } = getAPIProxy()
  return ethTxManagerProxy
}

export async function getNFTMetadata (token: BraveWallet.BlockchainToken) {
  const { jsonRpcService } = getAPIProxy()
  if (token.coin === BraveWallet.CoinType.ETH) {
    return await jsonRpcService.getERC721Metadata(token.contractAddress, token.tokenId, token.chainId)
  } else if (token.coin === BraveWallet.CoinType.SOL) {
    return await jsonRpcService.getSolTokenMetadata(token.chainId, token.contractAddress)
  }

  return undefined
}

export async function isTokenPinningSupported (token: BraveWallet.BlockchainToken) {
  const { braveWalletPinService } = getAPIProxy()
  return await braveWalletPinService.isTokenSupported(token)
}


export function refreshPortfolioFilterOptions () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const { accounts, selectedAccountFilter, selectedNetworkFilter } =
      getState().wallet

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

    if (
      selectedAccountFilter !== AllAccountsOption.id &&
      !accounts.some(account => account.id === selectedAccountFilter)
    ) {
      dispatch(WalletActions.setSelectedAccountFilterItem(AllAccountsOption.id))
      window.localStorage.removeItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ACCOUNT_FILTER_OPTION)
    }
  }
}

// Checks whether set of urls have ipfs:// scheme or are gateway-like urls
export const areSupportedForPinning = async (urls: string[]) => {
  const results =
    await Promise.all(
      urls.flatMap((v) => extractIpfsUrl(stripERC20TokenImageURL(v))))
  return results.every(result => result?.startsWith(IPFS_PROTOCOL))
}

// Extracts ipfs:// url from gateway-like url
export const extractIpfsUrl = async (url: string | undefined) => {
  const { braveWalletIpfsService } = getAPIProxy()
  const trimmedUrl = url ? url.trim() : ''
   if (isIpfs(trimmedUrl)) {
    return trimmedUrl
  }
  return (await braveWalletIpfsService
    .extractIPFSUrlFromGatewayLikeUrl(trimmedUrl))?.ipfsUrl || undefined
}

// Translates ipfs:// url or gateway-like url to the NFT gateway url
export const translateToNftGateway = async (url: string | undefined) => {
  const { braveWalletIpfsService } = getAPIProxy()
  const trimmedUrl = url ? url.trim() : ''
  const testUrl =
    isIpfs(trimmedUrl) ? trimmedUrl : await extractIpfsUrl(trimmedUrl)
  return (await braveWalletIpfsService
    .translateToNFTGatewayURL(testUrl || '')).translatedUrl || trimmedUrl
}

export const addLogoToToken = async (token: BraveWallet.BlockchainToken) => {
  const newLogo = token.logo?.startsWith('ipfs://')
    ? (await translateToNftGateway(token.logo))
    : token.logo?.startsWith('data:image/')
      ? token.logo
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

export const setNftDiscoveryEnabled = async (enabled: boolean) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.setNftDiscoveryEnabled(enabled)
}
