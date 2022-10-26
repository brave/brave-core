/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Constants
import {
  BraveWallet,
  SolFeeEstimates,
  TimeDelta,
  WalletAccountType
} from '../constants/types'

// Utils
import Amount from '../utils/amount'
import { TypedSolanaInstructionWithParams } from '../utils/solana-instruction-utils'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  getFormattedTransactionTransferedValue,
  getGasFee,
  getTransactionErc721TokenId,
  getTransactionGas,
  getTransactionGasLimit,
  getTransactionNonce,
  getTransactionToAddress,
  findTransactionToken,
  isEIP1559Transaction,
  isFilecoinTransaction,
  isMissingGasLimit,
  isSendingToKnownTokenContractAddress,
  isSolanaTransaction,
  isSwapTransaction,
  transactionHasSameAddressError,
  getTransactionIntent,
  findTransactionAccount
} from '../utils/tx-utils'

// Options
import { makeNetworkAsset } from '../options/asset-options'
import { getBalance } from '../utils/balance-utils'
import { getAddressLabel } from '../utils/account-utils'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  isEIP1559Transaction: boolean
  isMissingGasLimit?: boolean
  gasPremium?: string
  gasFeeCap?: string
}

interface ParsedTransactionFeesWithFiatPrices extends ParsedTransactionFees {
  gasFeeFiat: string
}

export interface ParsedTransactionBase extends ParsedTransactionFees {
  hash: string
  nonce: string
  createdTime: TimeDelta
  status: BraveWallet.TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  value: string
  valueExact: string
  symbol: string
  decimals: number
  insufficientFundsForGas?: boolean
  insufficientFunds?: boolean
  hasContractAddressError: boolean
  hasSameAddressError: boolean
  erc721BlockchainToken?: BraveWallet.BlockchainToken
  erc721TokenId?: string
  isSwap?: boolean
  intent: string
  originInfo?: BraveWallet.OriginInfo
  network?: BraveWallet.NetworkInfo
}

export interface ParsedTransaction extends ParsedTransactionBase {
  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: Amount
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: Amount

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]
}

export interface ParsedTransactionWithFiatPrices extends ParsedTransaction, ParsedTransactionFeesWithFiatPrices {
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
}

export const parseTransactionFeesWithoutPrices = (
  transactionInfo: BraveWallet.TransactionInfo,
  solFeeEstimates?: SolFeeEstimates
) => {
  const gasLimit = getTransactionGasLimit(transactionInfo)
  const {
    gasPrice,
    maxFeePerGas,
    maxPriorityFeePerGas
  } = getTransactionGas(transactionInfo)

  const gasFee = getGasFee(transactionInfo, solFeeEstimates)

  return {
    gasLimit: Amount.normalize(gasLimit),
    gasPrice: Amount.normalize(gasPrice),
    maxFeePerGas: Amount.normalize(maxFeePerGas),
    maxPriorityFeePerGas: Amount.normalize(maxPriorityFeePerGas),
    gasFee,
    isEIP1559Transaction: isEIP1559Transaction(transactionInfo),
    isMissingGasLimit: isSolanaTransaction(transactionInfo)
      ? undefined
      : isMissingGasLimit(gasLimit),
    gasPremium: isFilecoinTransaction(transactionInfo)
      ? new Amount(transactionInfo.txDataUnion.filTxData.gasPremium).format()
      : '',
    gasFeeCap: isFilecoinTransaction(transactionInfo)
      ? new Amount(transactionInfo.txDataUnion.filTxData.gasFeeCap).format()
      : ''
  }
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
  transactionNetwork: BraveWallet.NetworkInfo
  userVisibleTokensList: BraveWallet.BlockchainToken[]
}): ParsedTransaction {
  const nativeAsset = makeNetworkAsset(transactionNetwork)
  const { fromAddress } = tx
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const nonce = getTransactionNonce(tx)
  const toAddress = getTransactionToAddress(tx)
  const token = findTransactionToken(tx, combinedTokensList)
  const {
    value,
    valueExact,
    gweiValue
  } = getFormattedTransactionTransferedValue({
    fromAddress,
    tx: tx,
    txNetwork: transactionNetwork,
    token
  })

  const account = findTransactionAccount(accounts, tx)
  const accountNativeBalance = getBalance([transactionNetwork], account, nativeAsset)
  const accountTokenBalance = getBalance([transactionNetwork], account, token)

  const {
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isEIP1559Transaction,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas
  } = parseTransactionFeesWithoutPrices(
    tx,
    solFeeEstimates
  )

  const isSwap = isSwapTransaction(tx)

  const erc721Token = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  // base tx
  const parsedTxBase: ParsedTransactionBase = {
    originInfo: tx.originInfo,
    value,
    valueExact,
    hash: tx.txHash,
    nonce,
    createdTime: tx.createdTime,
    status: tx.txStatus,
    sender: fromAddress,
    recipient: toAddress,
    symbol: token?.symbol || '',
    decimals: transactionNetwork?.decimals ?? 18,
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isEIP1559Transaction,
    maxFeePerGas,
    maxPriorityFeePerGas,
    erc721BlockchainToken: erc721Token,
    isMissingGasLimit,
    isSwap,
    senderLabel: getAddressLabel(fromAddress, accounts),
    recipientLabel: getAddressLabel(toAddress, accounts),
    insufficientFundsForGas: accountHasInsufficientFundsForGas({
      accountNativeBalance,
      gasFee
    }),
    hasContractAddressError: isSendingToKnownTokenContractAddress(tx, toAddress, combinedTokensList),
    erc721TokenId: getTransactionErc721TokenId(tx),
    hasSameAddressError: transactionHasSameAddressError(tx, toAddress, fromAddress),
    insufficientFunds: accountHasInsufficientFundsForTransaction({
      account,
      accountNativeBalance,
      accountTokenBalance,
      gasFee,
      nativeAsset,
      tokensList: combinedTokensList,
      tx,
      transactionNetwork,
      transferedValue: gweiValue
    }),
    intent: getTransactionIntent(
      tx,
      transactionNetwork,
      combinedTokensList
    ),
    network: transactionNetwork
  }

  return parsedTxBase
}
