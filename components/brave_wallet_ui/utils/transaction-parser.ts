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
import { getTypedSolanaTxInstructions, TypedSolanaInstructionWithParams } from '../utils/solana-instruction-utils'
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
  findTransactionAccount,
  getETHSwapTranasactionBuyAndSellTokens,
  getTransactionApprovalTargetAddress,
  getIsTxApprovalUnlimited,
  isSolanaSplTransaction,
  getTransactionTokenSymbol
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
  createdTime: TimeDelta
  decimals: number
  erc721BlockchainToken?: BraveWallet.BlockchainToken
  erc721TokenId?: string
  hasContractAddressError: boolean
  hash: string
  hasSameAddressError: boolean
  insufficientFunds?: boolean
  insufficientFundsForGas?: boolean
  intent: string
  isSwap?: boolean
  network: BraveWallet.NetworkInfo
  nonce: string
  originInfo?: BraveWallet.OriginInfo
  recipient: string
  recipientLabel: string
  sender: string
  senderLabel: string
  status: BraveWallet.TransactionStatus
  symbol: string
  value: string
  valueExact: string

  isFilTransaction: boolean
  isSolanaTransaction: boolean
  isSPLTransaction: boolean
  accountAddress?: string
}

export interface ParsedTransaction extends ParsedTransactionBase {
  // Token approvals
  approvalTarget?: string
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
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const nonce = getTransactionNonce(tx)
  const toAddress = getTransactionToAddress(tx)

  const token = findTransactionToken(tx, combinedTokensList)
  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount
  } = tx.txType === BraveWallet.TransactionType.ETHSwap
    ? getETHSwapTranasactionBuyAndSellTokens({
        nativeAsset,
        tokensList: combinedTokensList,
        tx
      })
    : {
        buyToken: undefined,
        sellToken: undefined,
        buyAmount: new Amount(''),
        sellAmount: new Amount('')
      }

  const {
    value,
    valueExact,
    gweiValue
  } = getFormattedTransactionTransferedValue({
    tx: tx,
    txNetwork: transactionNetwork,
    token,
    buyToken,
    sellToken
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

  const erc721Token = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  return {
    approvalTarget: getTransactionApprovalTargetAddress(tx),
    createdTime: tx.createdTime,
    decimals: transactionNetwork?.decimals ?? 18, // TODO: make function for this logic
    erc721BlockchainToken: erc721Token,
    erc721TokenId: getTransactionErc721TokenId(tx),
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    hasContractAddressError: isSendingToKnownTokenContractAddress(tx, toAddress, combinedTokensList),
    hash: tx.txHash,
    hasSameAddressError: transactionHasSameAddressError(tx),
    insufficientFunds: accountHasInsufficientFundsForTransaction({ account, accountNativeBalance, accountTokenBalance, gasFee, nativeAsset, tokensList: combinedTokensList, tx, transactionNetwork, transferedValue: gweiValue }),
    insufficientFundsForGas: accountHasInsufficientFundsForGas({ accountNativeBalance, gasFee }),
    intent: getTransactionIntent(tx, transactionNetwork, combinedTokensList),
    isEIP1559Transaction,
    isMissingGasLimit,
    isSwap: isSwapTransaction(tx),
    maxFeePerGas,
    maxPriorityFeePerGas,
    network: transactionNetwork,
    nonce,
    originInfo: tx.originInfo,
    recipient: toAddress,
    recipientLabel: getAddressLabel(toAddress, accounts),
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol: getTransactionTokenSymbol({ tx, txNetwork: transactionNetwork, sellToken, token }),
    value,
    valueExact,
    buyToken,
    instructions: getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData),
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    minBuyAmount: buyAmount,
    sellAmount,
    sellToken,
    isFilTransaction: isFilecoinTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSPLTransaction: isSolanaSplTransaction(tx),
    accountAddress: account?.address
  }
}
