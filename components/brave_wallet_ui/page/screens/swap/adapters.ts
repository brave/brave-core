// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { ThunkDispatch } from '@reduxjs/toolkit'
import {
  ApproveERC20Params,
  BlockchainToken,
  ETHSendTransactionParams,
  JupiterQuoteParams,
  JupiterQuoteResponseWithError,
  JupiterSwapParams,
  JupiterSwapResponseWithError,
  NetworkInfo,
  SOLSendTransactionParams,
  SwapFee,
  WalletAccount,
  ZeroExQuoteResponseWithError,
  ZeroExSwapParams,
  ZeroExSwapResponseWithError
} from '@brave/swap-interface'
import {
  BraveWallet,
  SendEthTransactionParams,
  SolanaSerializedTransactionParams,
  WalletAccountType
} from '../../../constants/types'
import getAPIProxy from '../../../common/async/bridge'
import { getNetworkLogo } from '../../../options/asset-options'
import { walletApi } from '../../../common/slices/api.slice'

function getTokenParam (token: BlockchainToken): string {
  if (token.coingeckoId) {
    return token.coingeckoId
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID

  if (!isEthereumNetwork) {
    return token.symbol.toLowerCase()
  }

  if (token.contractAddress === '') {
    return token.symbol.toLowerCase()
  }

  return token.contractAddress
}

export function makeGetTokenPrice (toAssetSymbol: string) {
  async function getTokenPrice (token: BlockchainToken) {
    const { assetRatioService } = getAPIProxy()

    const param = getTokenParam(token)
    const { success, values } = await assetRatioService.getPrice(
      [param],
      [toAssetSymbol.toLowerCase()],
      BraveWallet.AssetPriceTimeframe.OneDay
    )

    if (success) {
      return values[0].price
    }

    throw new Error(`Failed to fetch price: ${param}`)
  }

  return getTokenPrice
}

export function makeNetworkInfo (network: BraveWallet.NetworkInfo): NetworkInfo {
  return {
    chainId: network.chainId,
    chainName: network.chainName,
    blockExplorerUrl: network.blockExplorerUrls[0],
    logo: getNetworkLogo(network.chainId, network.symbol),
    symbol: network.symbol,
    symbolName: network.symbolName,
    decimals: network.decimals,
    coin: network.coin,
    isEIP1559: network.isEip1559
  }
}

export function makeWalletAccount (account: WalletAccountType): WalletAccount {
  return {
    name: account.name,
    address: account?.address || '',
    coin: account.coin
  }
}

export function makeBlockchainToken (token: BraveWallet.BlockchainToken): BlockchainToken {
  return {
    contractAddress: token.contractAddress,
    name: token.name,
    logo: token.logo,
    isToken: token.coin === BraveWallet.CoinType.ETH ? token.isErc20 : token.contractAddress !== '',
    visible: token.visible,
    decimals: token.decimals,
    symbol: token.symbol,
    coingeckoId: token.coingeckoId,
    chainId: token.chainId,
    coin: token.coin
  }
}

export function makeSwapService () {
  async function getZeroExPriceQuote (params: ZeroExSwapParams): Promise<ZeroExQuoteResponseWithError> {
    const { swapService } = getAPIProxy()
    const { response, errorResponse, errorString } = await swapService.getPriceQuote(params)

    if (response) {
      return { response }
    }

    if (errorResponse) {
      return { errorResponse }
    }

    throw new Error(errorString)
  }

  async function getZeroExTransactionPayload (params: ZeroExSwapParams): Promise<ZeroExSwapResponseWithError> {
    const { swapService } = getAPIProxy()
    const { response, errorResponse, errorString } = await swapService.getTransactionPayload(params)

    if (response) {
      return { response }
    }

    if (errorResponse) {
      return { errorResponse }
    }

    throw new Error(errorString)
  }

  async function getJupiterQuote (params: JupiterQuoteParams): Promise<JupiterQuoteResponseWithError> {
    const { swapService } = getAPIProxy()
    const { response, errorResponse, errorString } = await swapService.getJupiterQuote(params)

    if (response) {
      return { response }
    }

    if (errorResponse) {
      return { errorResponse }
    }

    throw new Error(errorString)
  }

  async function getJupiterTransactionsPayload (params: JupiterSwapParams): Promise<JupiterSwapResponseWithError> {
    const { swapService } = getAPIProxy()
    const { response, errorResponse, errorString } = await swapService.getJupiterSwapTransactions(params)

    if (response) {
      return { response }
    }

    if (errorResponse) {
      return { errorResponse }
    }

    throw new Error(errorString)
  }

  async function isSwapSupported (chainId: string) {
    const { swapService } = getAPIProxy()
    return (await swapService.isSwapSupported(chainId)).result
  }

  async function getBraveFeeForAsset (asset: BlockchainToken) {
    if (asset.coin === BraveWallet.CoinType.ETH) {
      return {
        fee: '0.875',
        discount: '0'
      } as SwapFee
    }

    const { swapService } = getAPIProxy()
    const hasFee = (await swapService.hasJupiterFeesForTokenMint(asset.contractAddress)).result

    return {
      fee: '0.85',
      discount: hasFee ? '0' : '100'
    } as SwapFee
  }

  return {
    getZeroExPriceQuote,
    getZeroExTransactionPayload,
    getJupiterQuote,
    getJupiterTransactionsPayload,
    isSwapSupported,
    getBraveFeeForAsset
  }
}

export function makeGetGasPrice () {
  async function getGasPrice () {
    // FIXME: return the result of eth_gasPrice RPC
    return ''
  }

  return getGasPrice
}

export function makeGetGasPrice1559 () {
  async function getGasPrice1559 () {
    const { ethTxManagerProxy } = getAPIProxy()
    const result = await ethTxManagerProxy.getGasEstimation1559()
    if (result && result.estimation) {
      return result.estimation
    } else {
      throw Error('Failed to fetch EIP-1559 gas-pricing estimates')
    }
  }

  return getGasPrice1559
}

export function makeGetERC20ApproveData () {
  // FIXME - Remove contractAddress from ApproveERC20Params
  async function getERC20ApproveData (params: ApproveERC20Params) {
    const { ethTxManagerProxy } = getAPIProxy()
    const { success, data } = await ethTxManagerProxy.makeERC20ApproveData(
      params.spenderAddress,
      params.allowance
    )
    if (success) {
      return data
    } else {
      throw Error('Failed to make ERC20 approve data')
    }
  }

  return getERC20ApproveData
}

export function makeETHSendTransaction (
  func: (
    payload: SendEthTransactionParams
  ) => Promise<{ success: boolean, txMetaId: string, errorMessage: string }>,
  hasEIP1559Support: boolean
) {
  async function sendTransaction (params: ETHSendTransactionParams) {
    const { errorMessage } = await func({
      ...params,
      hasEIP1559Support,
      coin: BraveWallet.CoinType.ETH
    })

    if (errorMessage) {
      throw new Error(errorMessage)
    }
  }

  return sendTransaction
}

export function makeSOLSendTransaction (
  func: (
    params: SolanaSerializedTransactionParams
  ) => Promise<{ success: boolean, txMetaId: string, errorMessage: string }>
) {
  async function sendTransaction (params: SOLSendTransactionParams) {
    const { errorMessage } = await func({
      encodedTransaction: params.encodedTransaction,
      from: params.from,
      txType: BraveWallet.TransactionType.SolanaSwap,
      sendOptions: {
        maxRetries:
          params.sendOptions?.maxRetries !== undefined
            ? ({
              maxRetries: params.sendOptions.maxRetries
            } as unknown as BraveWallet.OptionalMaxRetries)
            : undefined,
        preflightCommitment: params.sendOptions?.preflightCommitment,
        skipPreflight:
          params.sendOptions?.skipPreflight !== undefined
            ? { skipPreflight: params.sendOptions.skipPreflight }
            : undefined
      }

      // TODO: support groupId
    })

    if (errorMessage) {
      throw new Error(errorMessage)
    }
  }

  return sendTransaction
}

export function makeSwitchAccount () {
  async function switchAccount (account: WalletAccount) {
    const { keyringService } = getAPIProxy()
    await keyringService.setSelectedAccount(account.address, account.coin)
  }

  return switchAccount
}

export function makeSwitchNetwork(dispatch: ThunkDispatch<any, any, any>) {
  async function switchNetwork({ chainId, coin }: NetworkInfo) {
    await dispatch(
      walletApi.endpoints.setNetwork.initiate({
        chainId,
        coin
      })
    )
    return undefined
  }

  return switchNetwork
}
