// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

import {
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  BraveWallet,
  CoinType,
  GetBlockchainTokenInfoReturnInfo,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SolanaSerializedTransactionParams,
  SupportedOnRampNetworks
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import {
  hasEIP1559Support
} from '../../utils/network-utils'
import { getAccountType } from '../../utils/account-utils'
import { getAssetIdKey, getBatTokensFromList, getNativeTokensFromList, getUniqueAssets } from '../../utils/asset-utils'
import { getVisibleNetworksList } from '../slices/api.slice'
import { makeNetworkAsset } from '../../options/asset-options'

import getAPIProxy from './bridge'
import { Dispatch, State } from './types'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import { GetAccountsHardwareOperationResult, SolDerivationPaths } from '../hardware/types'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import { AllNetworksOption, AllNetworksOptionDefault } from '../../options/network-filter-options'
import { AllAccountsOptionUniqueKey, applySelectedAccountFilter } from '../../options/account-filter-options'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
import { WalletPageActions } from '../../page/actions'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'
import { IPFS_PROTOCOL, isIpfs, stripERC20TokenImageURL } from '../../utils/string-utils'
import { toTxDataUnion } from '../../utils/tx-utils'

export const getERC20Allowance = (
  contractAddress: string,
  ownerAddress: string,
  spenderAddress: string,
  chainId: string,
): Promise<string> => {
  return new Promise(async (resolve, reject) => {
    const { jsonRpcService } = getAPIProxy()
    const result = await jsonRpcService.getERC20TokenAllowance(
      contractAddress,
      ownerAddress,
      spenderAddress,
      chainId
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
  const { keyringService } = getAPIProxy()
  const { accounts }= (await keyringService.getAllAccounts()).allAccounts
  for (const account of accounts) {
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
  stripeAssetOptions: BraveWallet.BlockchainToken[]
  coinbaseAssetOptions: BraveWallet.BlockchainToken[]
  allAssetOptions: BraveWallet.BlockchainToken[]
}> => {
  const { blockchainRegistry } = getAPIProxy()
  const { kRamp, kSardine, kTransak, kStripe, kCoinbase } = BraveWallet.OnRampProvider

  const rampAssets = await mapLimit(
    SupportedOnRampNetworks,
    10,
    async (chainId: string) =>
      await blockchainRegistry.getBuyTokens(kRamp, chainId)
  )

  const sardineAssets = await mapLimit(
    SupportedOnRampNetworks,
    10,
    async (chainId: string) =>
      await blockchainRegistry.getBuyTokens(kSardine, chainId)
  )

  const transakAssets = await mapLimit(
    SupportedOnRampNetworks,
    10,
    async (chainId: string) =>
      await blockchainRegistry.getBuyTokens(kTransak, chainId)
  )
  const stripeAssets = await mapLimit(
    SupportedOnRampNetworks,
    10,
    async (chainId: string) =>
      await blockchainRegistry.getBuyTokens(kStripe, chainId)
  )

  const coinbaseAssets = await mapLimit(
    SupportedOnRampNetworks,
    10,
    async (chainId: string) =>
      await blockchainRegistry.getBuyTokens(kCoinbase, chainId)
  )

  // add token logos
  const rampAssetOptions: BraveWallet.BlockchainToken[] = await mapLimit(
    rampAssets.flatMap((p) => p.tokens),
    10,
    async (token: BraveWallet.BlockchainToken) => await addLogoToToken(token)
  )

  const sardineAssetOptions: BraveWallet.BlockchainToken[] = await mapLimit(
    sardineAssets.flatMap((p) => p.tokens),
    10,
    async (token: BraveWallet.BlockchainToken) => await addLogoToToken(token)
  )

  const transakAssetOptions: BraveWallet.BlockchainToken[] = await mapLimit(
    transakAssets.flatMap((p) => p.tokens),
    10,
    async (token: BraveWallet.BlockchainToken) => await addLogoToToken(token)
  )

  const stripeAssetOptions: BraveWallet.BlockchainToken[] = await mapLimit(
    stripeAssets.flatMap((p) => p.tokens),
    10,
    async (token: BraveWallet.BlockchainToken) => await addLogoToToken(token)
  )

  const coinbaseAssetOptions: BraveWallet.BlockchainToken[] = await mapLimit(
    coinbaseAssets.flatMap((p) => p.tokens),
    10,
    async (token: BraveWallet.BlockchainToken) => await addLogoToToken(token)
  )

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

  const {
    tokens: stripeTokenOptions,
    nativeAssets: stripeNativeAssetOptions
  } = getNativeTokensFromList(stripeAssetOptions)

  const {
    tokens: coinbaseTokenOptions,
    nativeAssets: coinbaseNativeAssetOptions
  } = getNativeTokensFromList(coinbaseAssetOptions)

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

  const {
    bat: stripeBatTokens,
    nonBat: stripeNonBatTokens
  } = getBatTokensFromList(stripeTokenOptions)

  const {
    bat: coinbaseBatTokens,
    nonBat: coinbaseNonBatTokens
  } = getBatTokensFromList(coinbaseTokenOptions)

  // sort lists
  // Move Gas coins and BAT to front of list
  const sortedRampOptions = [...rampNativeAssetOptions, ...rampBatTokens, ...rampNonBatTokens]
  const sortedSardineOptions = [...sardineNativeAssetOptions, ...sardineBatTokens, ...sardineNonBatTokens]
  const sortedTransakOptions = [...transakNativeAssetOptions, ...transakBatTokens, ...transakNonBatTokens]
  const sortedStripeOptions = [...stripeNativeAssetOptions, ...stripeBatTokens, ...stripeNonBatTokens]
  const sortedCoinbaseOptions = [...coinbaseNativeAssetOptions, ...coinbaseBatTokens, ...coinbaseNonBatTokens]

  const results = {
    rampAssetOptions: sortedRampOptions,
    sardineAssetOptions: sortedSardineOptions,
    transakAssetOptions: sortedTransakOptions,
    stripeAssetOptions: sortedStripeOptions,
    coinbaseAssetOptions: sortedCoinbaseOptions,
    allAssetOptions: getUniqueAssets([
      ...sortedRampOptions,
      ...sortedSardineOptions,
      ...sortedTransakOptions,
      ...sortedStripeOptions,
      ...sortedCoinbaseOptions
    ])
  }

  return results
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
      const nativeAsset = makeNetworkAsset(network)

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
      : await mapLimit(
          networkList,
          10,
          async (item: BraveWallet.NetworkInfo) => await inner(item)
        )

    const removedAssetIds =
      [
        ...getState().wallet.removedFungibleTokenIds,
        ...getState().wallet.removedNonFungibleTokenIds
      ]
    const userVisibleTokensInfo = visibleAssets
      .flat(1)
      .filter(token => !removedAssetIds.includes(getAssetIdKey(token)))
    const removedNfts = visibleAssets
      .flat(1)
      .filter(token => removedAssetIds.includes(getAssetIdKey(token)))
    await dispatch(WalletActions.setVisibleTokensInfo(userVisibleTokensInfo))
    await dispatch(WalletActions.setRemovedNonFungibleTokens(removedNfts))
    const nfts = userVisibleTokensInfo.filter((asset) => asset.isErc721 || asset.isNft)
    dispatch(WalletPageActions.getNftsPinningStatus(nfts))
  }
}

export function refreshSitePermissions () {
  return async (dispatch: Dispatch, getState: () => State) => {
    const apiProxy = getAPIProxy()
    const { braveWalletService } = apiProxy

    const { wallet: { accounts } } = getState()

    // Get a list of accounts with permissions of the active origin
    const { accountsWithPermission } =
      await braveWalletService.hasPermission(accounts.map((acc) => acc.accountId))

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
      isEIP1559 = hasEIP1559Support(getAccountType(payload.fromAccount), payload.network)
  }

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
      chainId: payload.network.chainId,
      // Estimated by eth_tx_service if value is ''
      maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
      // Estimated by eth_tx_service if value is ''
      maxFeePerGas: payload.maxFeePerGas || '',
      gasEstimation: undefined
    }
    return await apiProxy.txService.addUnapprovedTransaction(
      toTxDataUnion({ ethTxData1559: txData1559 }),
      payload.fromAccount.address,
      null,
      null
    )
  }

  return await apiProxy.txService.addUnapprovedTransaction(
    toTxDataUnion({ ethTxData: txData }),
    payload.fromAccount.address,
    null,
    null
  )
}

export async function sendFilTransaction(payload: SendFilTransactionParams) {
  const apiProxy = getAPIProxy()
  const filTxData: BraveWallet.FilTxData = {
    nonce: payload.nonce || '',
    gasPremium: payload.gasPremium || '',
    gasFeeCap: payload.gasFeeCap || '',
    gasLimit: payload.gasLimit || '',
    maxFee: payload.maxFee || '0',
    to: payload.to,
    value: payload.value
  }
  return await apiProxy.txService.addUnapprovedTransaction(
    toTxDataUnion({ filTxData: filTxData }),
    payload.fromAccount.address,
    null,
    null
  )
}

export async function sendSolTransaction(payload: SendSolTransactionParams) {
  const { solanaTxManagerProxy, txService } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeSystemProgramTransferTxData(
    payload.fromAccount.address,
    payload.to,
    BigInt(payload.value)
  )
  return await txService.addUnapprovedTransaction(
    toTxDataUnion({ solanaTxData: value.txData ?? undefined }),
    payload.fromAccount.address,
    null,
    null
  )
}

export async function sendSPLTransaction (payload: BraveWallet.SolanaTxData) {
  const { txService } = getAPIProxy()
  return await txService.addUnapprovedTransaction(
    toTxDataUnion({ solanaTxData: payload }),
    payload.feePayer,
    null,
    null
  )
}

export async function sendSolanaSerializedTransaction(
  payload: SolanaSerializedTransactionParams
) {
  const { solanaTxManagerProxy, txService } = getAPIProxy()
  const result =
    await solanaTxManagerProxy.makeTxDataFromBase64EncodedTransaction(
      payload.encodedTransaction,
      payload.txType,
      payload.sendOptions || null
    )
  if (result.error !== BraveWallet.ProviderError.kSuccess) {
    console.error(`Failed to sign Solana message: ${result.errorMessage}`)
    return { success: false, errorMessage: result.errorMessage, txMetaId: '' }
  } else {
    return await txService.addUnapprovedTransaction(
      toTxDataUnion({ solanaTxData: result.txData ?? undefined }),
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
  if (token.coin === CoinType.ETH) {
    return await jsonRpcService.getERC721Metadata(
      token.contractAddress, token.tokenId, token.chainId)
  } else if (token.coin === CoinType.SOL) {
    return await jsonRpcService.getSolTokenMetadata(
      token.chainId, token.contractAddress)
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

    if (!applySelectedAccountFilter(accounts, selectedAccountFilter).accounts) {
      dispatch(WalletActions.setSelectedAccountFilterItem(AllAccountsOptionUniqueKey))
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
