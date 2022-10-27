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
  getTransactionErc721TokenId,
  getTransactionNonce,
  getTransactionToAddress,
  findTransactionToken,
  isFilecoinTransaction,
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
  getTransactionTokenSymbol,
  parseTransactionFeesWithoutPrices,
  isSolanaDappTransaction
} from '../utils/tx-utils'

// Options
import { makeNetworkAsset } from '../options/asset-options'
import { getBalance } from '../utils/balance-utils'
import { getAddressLabel } from '../utils/account-utils'
import { getCoinFromTxDataUnion } from './network-utils'

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
  id: string
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
  weiValue: string
  coinType: BraveWallet.CoinType
  isFilTransaction: boolean
  isSolanaTransaction: boolean
  isSolanaDappTransaction: boolean
  isSPLTransaction: boolean
  accountAddress?: string
  txType: BraveWallet.TransactionType
  token?: BraveWallet.BlockchainToken
}

export interface ParsedTransaction extends ParsedTransactionBase {
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

export interface ParsedTransactionWithFiatPrices extends ParsedTransaction, ParsedTransactionFeesWithFiatPrices {
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
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
    sellAmount,
    sellAmountWei,
    buyAmountWei
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
        sellAmount: new Amount(''),
        sellAmountWei: new Amount(''),
        buyAmountWei: new Amount('')
      }

  const {
    value,
    valueExact,
    weiValue
  } = getFormattedTransactionTransferedValue({
    tx: tx,
    txNetwork: transactionNetwork,
    token,
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

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  return {
    accountAddress: account?.address,
    approvalTarget,
    approvalTargetLabel: getAddressLabel(approvalTarget, accounts),
    buyToken,
    coinType: getCoinFromTxDataUnion(tx.txDataUnion),
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
    id: tx.id,
    instructions: getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData),
    insufficientFunds: accountHasInsufficientFundsForTransaction({ account, accountNativeBalance, accountTokenBalance, gasFee, nativeAsset, tokensList: combinedTokensList, tx, transactionNetwork, transferedValue: weiValue }),
    insufficientFundsForGas: accountHasInsufficientFundsForGas({ accountNativeBalance, gasFee }),
    intent: getTransactionIntent(tx, transactionNetwork, combinedTokensList),
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilTransaction: isFilecoinTransaction(tx),
    isMissingGasLimit,
    isSolanaDappTransaction: isSolanaDappTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSPLTransaction: isSolanaSplTransaction(tx),
    isSwap: isSwapTransaction(tx),
    maxFeePerGas,
    maxPriorityFeePerGas,
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    network: transactionNetwork,
    nonce,
    originInfo: tx.originInfo,
    recipient: toAddress,
    recipientLabel: getAddressLabel(toAddress, accounts),
    sellAmount,
    sellAmountWei,
    sellToken,
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol: getTransactionTokenSymbol({ tx, txNetwork: transactionNetwork, sellToken, token }),
    txType: tx.txType,
    value,
    valueExact,
    weiValue,
    token
  }
}
