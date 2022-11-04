/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  SolFeeEstimates,
  TimeDelta,
  WalletAccountType,
  WalletState
} from '../../constants/types'
import { SwapExchangeProxy } from '../constants/registry'

// Utils
import { getLocale } from '../../../common/locale'
import Amount from '../../utils/amount'
import {
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from '../../utils/solana-instruction-utils'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionAccount,
  findTransactionToken,
  getETHSwapTranasactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getGasFeeFiatValue,
  getIsTxApprovalUnlimited,
  getTransactionApprovalTargetAddress,
  getTransactionDecimals,
  getTransactionErc721TokenId,
  getTransactionFiatValues,
  getTransactionIntent,
  getTransactionNonce,
  getTransactionToAddress,
  getTransactionTokenSymbol,
  isFilecoinTransaction,
  isSendingToKnownTokenContractAddress,
  isSolanaSplTransaction,
  isSolanaTransaction,
  isSwapTransaction,
  parseTransactionFeesWithoutPrices,
  transactionHasSameAddressError
} from '../../utils/tx-utils'
import { getBalance } from '../../utils/balance-utils'
import { getAddressLabel } from '../../utils/account-utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'

// Hooks
import usePricing from './pricing'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
  missingGasLimitError?: string
  gasPremium?: string
  gasFeeCap?: string
}

export interface ParsedTransaction extends ParsedTransactionFees {
  // Common fields
  id: string
  hash: string
  nonce: string
  createdTime: TimeDelta
  status: BraveWallet.TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  value: string
  valueExact: string
  symbol: string
  decimals: number // network decimals
  insufficientFundsForGasError?: boolean
  insufficientFundsError?: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721TokenId?: string
  isSwap?: boolean
  intent: string

  // Tx type flags
  isSolanaTransaction: boolean
  isSolanaDappTransaction: boolean
  isSolanaSPLTransaction: boolean
  isFilecoinTransaction: boolean
  coinType: BraveWallet.CoinType

  // Tokens
  token?: BraveWallet.BlockchainToken
  erc721BlockchainToken?: BraveWallet.BlockchainToken

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: Amount
  sellAmountWei?: Amount
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: Amount
  minBuyAmountWei?: Amount

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]
}

export function useTransactionFeesParser (selectedNetwork?: BraveWallet.NetworkInfo, networkSpotPrice?: string, solFeeEstimates?: SolFeeEstimates) {
  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransactionFees => {
    const txFeesBase = parseTransactionFeesWithoutPrices(
      transactionInfo,
      solFeeEstimates
    )

    return {
      ...txFeesBase,
      gasFeeFiat: getGasFeeFiatValue({
        gasFee: txFeesBase.gasFee,
        networkSpotPrice,
        txNetwork: selectedNetwork
      }),
      missingGasLimitError: txFeesBase.isMissingGasLimit
        ? getLocale('braveWalletMissingGasLimitError')
        : undefined
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  transactionNetwork?: BraveWallet.NetworkInfo
) {
  const {
    selectedNetwork: reduxSelectedNetwork,
    fullTokenList,
    userVisibleTokensInfo: visibleTokens,
    accounts,
    transactionSpotPrices: spotPrices,
    solFeeEstimates
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const selectedNetwork = transactionNetwork || reduxSelectedNetwork
  const nativeAsset = React.useMemo(
    () => selectedNetwork && makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )
  const { findAssetPrice, computeFiatAmount } = usePricing(spotPrices)

  const networkSpotPrice = React.useMemo(
    () => selectedNetwork
      ? findAssetPrice(selectedNetwork.symbol)
      : '',
    [selectedNetwork, findAssetPrice]
  )

  const combinedTokensList = React.useMemo(() => {
    return visibleTokens.concat(fullTokenList)
  }, [visibleTokens, fullTokenList])

  return React.useCallback((tx: BraveWallet.TransactionInfo): ParsedTransaction => {
    const { txType } = tx

    const { gasFee } = parseTransactionFeesWithoutPrices(tx, solFeeEstimates)
    const gasFeeFiat = getGasFeeFiatValue({
      gasFee,
      networkSpotPrice,
      txNetwork: transactionNetwork
    })

    const isSPLTransaction = isSolanaSplTransaction(tx)
    const to = getTransactionToAddress(tx)
    const token = findTransactionToken(tx, combinedTokensList)

    const { sellToken } = getETHSwapTranasactionBuyAndSellTokens({
      nativeAsset,
      tokensList: combinedTokensList,
      tx
    })

    const {
      normalizedTransferredValue,
      normalizedTransferredValueExact,
      weiTransferredValue
    } = getFormattedTransactionTransferredValue({
      tx,
      txNetwork: transactionNetwork,
      token,
      sellToken
    })

    const txBase: Omit<
      ParsedTransaction,
      | 'fiatTotal'
      | 'fiatValue'
      | 'formattedNativeCurrencyTotal'
    > = {
      ...parseTransactionWithoutPrices({
        accounts,
        fullTokenList,
        transactionNetwork: selectedNetwork,
        tx,
        userVisibleTokensList: visibleTokens,
        solFeeEstimates
      }),
      gasFeeFiat
    }

    switch (true) {
      case txBase.isSolanaDappTransaction: {
        const transferedAmountFiat = selectedNetwork
          ? computeFiatAmount(
              normalizedTransferredValueExact,
              selectedNetwork.symbol,
              selectedNetwork.decimals
            )
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(transferedAmountFiat)

        const parsedTx: ParsedTransaction = {
          ...txBase,
          fiatValue: transferedAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: transferedAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol)
        }

        return parsedTx
      }

      // transfer(address recipient, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Transfer: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          sellToken,
          token,
          txNetwork: selectedNetwork
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721SafeTransferFrom: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          sellToken,
          token,
          txNetwork: selectedNetwork
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Approve: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          sellToken,
          token,
          txNetwork: selectedNetwork
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }

      case isSPLTransaction: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          sellToken,
          token,
          txNetwork: selectedNetwork
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }

      // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
      case txType === BraveWallet.TransactionType.ETHSwap: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          sellAmountWei: weiTransferredValue,
          sellToken,
          token,
          txNetwork: selectedNetwork
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }

      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
      case txType === BraveWallet.TransactionType.SolanaSystemTransfer:
      case txType === BraveWallet.TransactionType.Other:
      default: {
        const {
          fiatTotal,
          fiatValue,
          formattedNativeCurrencyTotal
        } = getTransactionFiatValues({
          gasFee,
          networkSpotPrice,
          normalizedTransferredValue,
          spotPrices,
          tx,
          txNetwork: selectedNetwork,
          sellToken,
          token
        })

        return {
          ...txBase,
          fiatValue,
          fiatTotal,
          formattedNativeCurrencyTotal
        } as ParsedTransaction
      }
    }
  }, [
    selectedNetwork,
    accounts,
    spotPrices
  ])
}

export function parseTransactionWithoutPrices ({
  accounts,
  fullTokenList,
  tx,
  transactionNetwork,
  userVisibleTokensList,
  solFeeEstimates
}: {
  accounts: WalletAccountType[]
  fullTokenList: BraveWallet.BlockchainToken[]
  solFeeEstimates?: SolFeeEstimates
  tx: BraveWallet.TransactionInfo
  transactionNetwork?: BraveWallet.NetworkInfo
  userVisibleTokensList: BraveWallet.BlockchainToken[]
}): ParsedTransaction {
  const to = getTransactionToAddress(tx)
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const token = findTransactionToken(tx, combinedTokensList)
  const nativeAsset = transactionNetwork ? makeNetworkAsset(transactionNetwork) : undefined
  const account = findTransactionAccount(accounts, tx)
  const accountNativeBalance = getBalance(account, nativeAsset)
  const accountTokenBalance = getBalance(account, token)

  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount,
    sellAmountWei,
    buyAmountWei
  } = getETHSwapTranasactionBuyAndSellTokens({
    nativeAsset,
    tokensList: combinedTokensList,
    tx
  })

  const {
    normalizedTransferredValue,
    normalizedTransferredValueExact
  } = getFormattedTransactionTransferredValue({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const erc721Token = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  const {
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas,
    isEIP1559Transaction
  } = parseTransactionFeesWithoutPrices(tx, solFeeEstimates)

  const insufficientFundsError = accountHasInsufficientFundsForTransaction({
    accountNativeBalance,
    accountTokenBalance,
    gasFee,
    tokensList: combinedTokensList,
    tx,
    account,
    nativeAsset
  })

  const erc721TokenId = getTransactionErc721TokenId(tx)

  return {
    gasFee,
    gasLimit,
    gasPrice,
    gasFeeCap,
    gasPremium,
    maxFeePerGas,
    maxPriorityFeePerGas,
    missingGasLimitError: isMissingGasLimit
      ? getLocale('braveWalletMissingGasLimitError')
      : undefined,
    approvalTarget,
    approvalTargetLabel: getAddressLabel(approvalTarget, accounts),
    buyToken,
    coinType: getCoinFromTxDataUnion(tx.txDataUnion),
    createdTime: tx.createdTime,
    contractAddressError: isSendingToKnownTokenContractAddress(tx, combinedTokensList)
      ? getLocale('braveWalletContractAddressError')
      : undefined,
    decimals: getTransactionDecimals({
      tx,
      network: transactionNetwork,
      sellToken,
      erc721Token,
      token
    }),
    erc721BlockchainToken: erc721Token,
    erc721TokenId,
    hash: tx.txHash,
    id: tx.id,
    instructions: getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData),
    insufficientFundsError,
    insufficientFundsForGasError: accountHasInsufficientFundsForGas({
      accountNativeBalance,
      gasFee
    }),
    intent: getTransactionIntent({
      normalizedTransferredValue,
      tx,
      buyAmount,
      buyToken,
      erc721TokenId,
      sellAmount,
      sellToken,
      token,
      transactionNetwork
    }),
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSwap: isSwapTransaction(tx),
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    nonce: getTransactionNonce(tx),
    recipient: to,
    recipientLabel: getAddressLabel(to, accounts),
    sameAddressError: transactionHasSameAddressError(tx)
      ? getLocale('braveWalletSameAddressError')
      : undefined,
    sellAmount,
    sellAmountWei,
    sellToken,
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol: getTransactionTokenSymbol({
      tx,
      txNetwork: transactionNetwork,
      token,
      sellToken
    }),
    token,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact
  } as ParsedTransaction
}
