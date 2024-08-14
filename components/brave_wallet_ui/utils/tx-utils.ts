// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assert, assertNotReached } from 'chrome://resources/js/assert.js'

import { EntityState } from '@reduxjs/toolkit'

// types
import {
  BraveWallet,
  SerializableTransactionInfo,
  TimeDelta,
  SerializableTimeDelta,
  SortingOrder,
  TransactionInfo,
  SpotPriceRegistry
} from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'
import {
  MAX_UINT256,
  NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
  UNKNOWN_TOKEN_COINGECKO_ID
} from '../common/constants/magics'
import { SwapExchangeProxy } from '../common/constants/registry'

// utils
import { getLocale } from '../../common/locale'
import {
  getSolInstructionAccountParamsObj,
  getSolInstructionParamsObj,
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from './solana-instruction-utils'
import { findTokenByContractAddress } from './asset-utils'
import Amount from './amount'
import { getCoinFromTxDataUnion, TxDataPresence } from './network-utils'
import { toProperCase } from './string-utils'
import {
  computeFiatAmount,
  getTokenPriceAmountFromRegistry
} from './pricing-utils'
import { makeNetworkAsset } from '../options/asset-options'
import { getAccountLabel, getAddressLabel } from './account-utils'
import { makeSerializableTimeDelta } from './model-serialization-utils'

export type FileCoinTransactionInfo = TransactionInfo & {
  txDataUnion: {
    filTxData: BraveWallet.FilTxData
    ethTxData1559: undefined
    ethTxData: undefined
    solanaTxData: undefined
  }
}

export type SolanaTransactionInfo = TransactionInfo & {
  txDataUnion: {
    solanaTxData: BraveWallet.SolanaTxData
    ethTxData1559: undefined
    ethTxData: undefined
    filTxData: undefined
  }
}

export interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
  missingGasLimitError?: string
  gasPremium?: string
  gasFeeCap?: string
}

export interface ParsedTransaction
  extends ParsedTransactionFees,
    Pick<BraveWallet.TransactionInfo, 'txType'> {
  // Common fields
  id: string
  hash: string
  nonce: string
  createdTime: SerializableTimeDelta
  status: BraveWallet.TransactionStatus
  senderLabel: string
  recipient: string
  recipientLabel: string
  symbol: string
  decimals: number // network decimals
  contractAddressError?: string
  sameAddressError?: string
  erc721TokenId?: string
  isSwap?: boolean
  intent: string
  chainId: string
  originInfo?: BraveWallet.OriginInfo | undefined

  // Value
  value: string
  valueExact: string
  weiTransferredValue: string
  formattedSendCurrencyTotal: string

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
  // sending to 0x Exchange Proxy
  isSendingToZeroXExchangeProxy: boolean

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]

  // Fiat values
  fiatValue: string
  fiatTotal: string

  // Solana Specific
  isAssociatedTokenAccountCreation: boolean
}

export type ParsedTransactionWithoutFiatValues = Omit<
  ParsedTransaction,
  'fiatTotal' | 'fiatValue' | 'gasFeeFiat'
>

export const sortTransactionByDate = <
  T extends { createdTime: TimeDelta | SerializableTimeDelta }
>(
  transactions: T[],
  order: SortingOrder = 'ascending'
): T[] => {
  return [...transactions].sort(transactionSortByDateComparer<T>(order))
}

export const getLocaleKeyForTxStatus = (
  status: BraveWallet.TransactionStatus
) => {
  switch (status) {
    case BraveWallet.TransactionStatus.Unapproved:
      return 'braveWalletTransactionStatusUnapproved'
    case BraveWallet.TransactionStatus.Approved:
      return 'braveWalletTransactionStatusApproved'
    case BraveWallet.TransactionStatus.Rejected:
      return 'braveWalletTransactionStatusRejected'
    case BraveWallet.TransactionStatus.Submitted:
      return 'braveWalletTransactionStatusSubmitted'
    case BraveWallet.TransactionStatus.Confirmed:
      return 'braveWalletTransactionStatusConfirmed'
    case BraveWallet.TransactionStatus.Error:
      return 'braveWalletTransactionStatusError'
    case BraveWallet.TransactionStatus.Dropped:
      return 'braveWalletTransactionStatusDropped'
    case BraveWallet.TransactionStatus.Signed:
      return 'braveWalletTransactionStatusSigned'
    default:
      return ''
  }
}

export const getTransactionStatusString = (statusId: number) => {
  return getLocale(getLocaleKeyForTxStatus(statusId))
}

export const transactionSortByDateComparer = <
  T extends { createdTime: TimeDelta | SerializableTimeDelta }
>(
  order: SortingOrder = 'ascending'
): ((a: T, b: T) => number) | undefined => {
  return function (x: T, y: T) {
    return order === 'ascending'
      ? Number(x.createdTime.microseconds) - Number(y.createdTime.microseconds)
      : Number(y.createdTime.microseconds) - Number(x.createdTime.microseconds)
  }
}

export function isSolanaTransaction(
  tx?: Pick<TransactionInfo, 'txType' | 'txDataUnion'>
): tx is SolanaTransactionInfo {
  if (!tx) {
    return false
  }
  const {
    txType,
    txDataUnion: { solanaTxData }
  } = tx
  return (
    SolanaTransactionTypes.includes(txType) ||
    (txType === BraveWallet.TransactionType.Other && solanaTxData !== undefined)
  )
}

export function isBitcoinTransaction(
  tx?: Pick<TransactionInfo, 'txDataUnion'>
) {
  if (!tx) {
    return false
  }
  return tx.txDataUnion.btcTxData !== undefined
}

export function isZCashTransaction(tx?: Pick<TransactionInfo, 'txDataUnion'>) {
  if (!tx) {
    return false
  }
  return tx.txDataUnion.zecTxData !== undefined
}

export function isEthereumTransaction(
  tx?: Pick<TransactionInfo, 'txDataUnion'>
) {
  if (!tx) {
    return false
  }
  return (
    tx.txDataUnion.ethTxData !== undefined ||
    tx.txDataUnion.ethTxData1559 !== undefined
  )
}

export const getTransactionNonce = (tx: TransactionInfo): string => {
  return tx.txDataUnion?.ethTxData1559?.baseData.nonce || ''
}

export function isSolanaDappTransaction(
  tx: TransactionInfo
): tx is SolanaTransactionInfo {
  return (
    tx.txDataUnion.solanaTxData !== undefined &&
    [
      BraveWallet.TransactionType.SolanaDappSignTransaction,
      BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
      BraveWallet.TransactionType.SolanaSwap,
      BraveWallet.TransactionType.Other
    ].includes(tx.txType)
  )
}

export const isFilecoinTransaction = (tx?: {
  txDataUnion: TxDataPresence
}): tx is FileCoinTransactionInfo => {
  return tx?.txDataUnion.filTxData !== undefined
}

function getTypedSolanaInstructionToAddress(
  to: string
): (value: TypedSolanaInstructionWithParams) => string {
  return (instruction) => {
    const { toAccount, newAccount } = getSolInstructionAccountParamsObj(
      instruction.accountParams,
      instruction.accountMetas
    )

    switch (instruction.type) {
      case 'Transfer':
      case 'TransferWithSeed':
      case 'CloseAccount':
      case 'WithdrawNonceAccount': {
        return toAccount ?? ''
      }

      case 'CreateAccount':
      case 'CreateAccountWithSeed': {
        return newAccount ?? ''
      }

      case undefined:
      default:
        return toAccount || newAccount || to || ''
    }
  }
}

export const getToAddressesFromSolanaTransaction = (
  tx: SolanaTransactionInfo
) => {
  const { solanaTxData } = tx.txDataUnion
  const to = solanaTxData?.toWalletAddress ?? ''

  if (to) {
    return [to]
  }

  const instructions = getTypedSolanaTxInstructions(solanaTxData)
  const addresses = instructions.map(getTypedSolanaInstructionToAddress(to))

  // unique, non empty addresses
  return [...new Set(addresses.filter((a) => !!a))]
}

export const getTransactionToAddress = (
  tx?: TransactionInfo | SerializableTransactionInfo
): string => {
  if (!tx) {
    return ''
  }

  if (isSolanaDappTransaction(tx)) {
    return getToAddressesFromSolanaTransaction(tx)[0] ?? ''
  }

  if (isEthereumTransaction(tx) || isFilecoinTransaction(tx)) {
    return tx.effectiveRecipient || ''
  }

  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData?.toWalletAddress ?? ''
  }

  if (isZCashTransaction(tx)) {
    return tx.txDataUnion.zecTxData?.to ?? ''
  }

  if (isBitcoinTransaction(tx)) {
    return tx.txDataUnion.btcTxData?.to ?? ''
  }

  assertNotReached('Unknown transaction type')
}

export function getTransactionInteractionAddress(
  tx: Pick<TransactionInfo, 'txDataUnion' | 'txType'>
): string {
  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.toWalletAddress ?? ''
  }

  if (isFilecoinTransaction(tx)) {
    return tx.txDataUnion.filTxData.to ?? ''
  }

  if (isZCashTransaction(tx)) {
    return tx.txDataUnion.zecTxData?.to ?? ''
  }

  if (isBitcoinTransaction(tx)) {
    return tx.txDataUnion.btcTxData?.to ?? ''
  }

  if (isEthereumTransaction(tx)) {
    return (
      tx.txDataUnion.ethTxData1559?.baseData.to || // EVM (1559)
      tx.txDataUnion.ethTxData?.to || // EVM
      '' // Other
    )
  }

  assertNotReached('Unknown transaction type')
}

export function isSolanaSplTransaction(
  tx: TransactionInfo
): tx is SolanaTransactionInfo {
  return (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType ===
      BraveWallet.TransactionType
        .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  )
}

export const findTransactionToken = <
  T extends Pick<
    BraveWallet.BlockchainToken,
    'contractAddress' | 'chainId' | 'coin'
  >
>(
  tx: TransactionInfo | undefined,
  tokensList: T[]
): T | undefined => {
  if (!tx) {
    return undefined
  }

  // Native Asset Send
  if (
    tx.txType === BraveWallet.TransactionType.SolanaSystemTransfer ||
    tx.txType === BraveWallet.TransactionType.ETHSend ||
    tx.txDataUnion.filTxData ||
    tx.txDataUnion.btcTxData ||
    tx.txDataUnion.zecTxData
  ) {
    return tokensList.find(
      (t) =>
        t.contractAddress === '' &&
        t.chainId === tx.chainId &&
        t.coin === tx.fromAccountId.coin
    )
  }

  // Solana SPL
  if (isSolanaSplTransaction(tx)) {
    return findTokenByContractAddress(
      tx.txDataUnion.solanaTxData.splTokenMintAddress ?? '',
      tokensList
    )
  }

  // Solana Dapps, Filecoin & EVM
  return findTokenByContractAddress(
    // tx interacts with the contract address
    getTransactionInteractionAddress(tx),
    tokensList
  )
}

/**
 * Use this function to synchronously extract swap/bridge details from a
 * transaction.
 *
 * Prefer using useSwapTransactionParser() hook in React components, which
 * can asynchronously extract details from the blockchain.
 */
export const getETHSwapTransactionBuyAndSellTokens = ({
  nativeAsset,
  tokensList,
  tx
}: {
  tx: TransactionInfo | undefined
  nativeAsset?: BraveWallet.BlockchainToken
  tokensList: BraveWallet.BlockchainToken[]
}): {
  buyToken?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
  buyAmount: Amount
  buyAmountWei: Amount
  sellAmount: Amount
  sellAmountWei: Amount
} => {
  if (
    !tx ||
    !tx.swapInfo ||
    tx.txType !== BraveWallet.TransactionType.ETHSwap
  ) {
    return {
      buyToken: undefined,
      sellToken: undefined,
      buyAmount: Amount.empty(),
      sellAmount: Amount.empty(),
      sellAmountWei: Amount.empty(),
      buyAmountWei: Amount.empty()
    }
  }

  const sellToken =
    tx.swapInfo.fromAsset === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? nativeAsset
      : findTokenByContractAddress(tx.swapInfo.fromAsset, tokensList) ||
        // token not found
        // return a "faked" coin (will need to "discover" it later)
        ({
          chainId: tx.swapInfo.fromChainId,
          coin: tx.swapInfo.fromCoin,
          contractAddress: tx.swapInfo.fromAsset,
          symbol: '???',
          isErc20: true,
          coingeckoId: UNKNOWN_TOKEN_COINGECKO_ID,
          name: tx.swapInfo.fromAsset,
          logo: 'chrome://erc-token-images/',
          tokenId: '',
          isErc1155: false,
          isErc721: false,
          isNft: false,
          isSpam: false,
          visible: true
        } as BraveWallet.BlockchainToken)

  const sellAmountWei = new Amount(tx.swapInfo.fromAmount)
  const sellAmount = sellToken
    ? sellAmountWei.divideByDecimals(sellToken.decimals)
    : Amount.empty()

  const buyToken =
    tx.swapInfo.toAsset === NATIVE_EVM_ASSET_CONTRACT_ADDRESS
      ? nativeAsset
      : tx.swapInfo.toAsset
      ? findTokenByContractAddress(tx.swapInfo.toAsset, tokensList) ||
        // token not found
        // return a "faked" coin (will need to "discover" it later)
        ({
          chainId: tx.swapInfo.toChainId,
          coin: tx.swapInfo.toCoin,
          contractAddress: tx.swapInfo.toAsset,
          symbol: '???',
          isErc20: true,
          coingeckoId: UNKNOWN_TOKEN_COINGECKO_ID,
          name: tx.swapInfo.toAsset,
          logo: 'chrome://erc-token-images/',
          tokenId: '',
          isErc1155: false,
          isErc721: false,
          isNft: false,
          isSpam: false,
          visible: true
        } as BraveWallet.BlockchainToken)
      : undefined

  const buyAmountWei = tx.swapInfo.toAmount
    ? new Amount(tx.swapInfo.toAmount)
    : Amount.empty()
  const buyAmount = buyToken
    ? buyAmountWei.divideByDecimals(buyToken.decimals)
    : Amount.empty()

  return {
    buyToken,
    sellToken,
    sellAmount,
    buyAmount,
    buyAmountWei,
    sellAmountWei
  }
}

export function getLamportsMovedFromInstructions(
  instructions: TypedSolanaInstructionWithParams[],
  fromAddress: string
) {
  return (
    instructions.reduce((acc, instruction) => {
      const { lamports } = getSolInstructionParamsObj(instruction.params)

      const { fromAccount, nonceAccount, toAccount } =
        getSolInstructionAccountParamsObj(
          instruction.accountParams,
          instruction.accountMetas
        )

      switch (instruction.type) {
        case 'Transfer':
        case 'TransferWithSeed': {
          // only show lamports as transferred if
          // the amount is going to a different pubKey
          if (toAccount !== fromAccount) {
            return acc.plus(lamports)
          }
          return acc
        }

        case 'WithdrawNonceAccount': {
          if (nonceAccount === fromAddress) {
            return acc.plus(lamports)
          }

          if (toAccount === fromAddress) {
            return acc.minus(lamports)
          }

          return acc
        }

        case 'CreateAccount':
        case 'CreateAccountWithSeed': {
          if (fromAccount === fromAddress) {
            return acc.plus(lamports)
          }

          return acc
        }

        default:
          return acc.plus(lamports)
      }
    }, new Amount(0)) ?? 0
  )
}

export function getTransactionBaseValue(tx: TransactionInfo) {
  if (isSolanaSplTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.amount.toString() ?? ''
  }

  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.lamports.toString() ?? ''
  }

  if (isFilecoinTransaction(tx)) {
    return tx.txDataUnion.filTxData.value || ''
  }

  if (isEthereumTransaction(tx)) {
    return tx.txDataUnion.ethTxData1559?.baseData.value || ''
  }

  if (isBitcoinTransaction(tx)) {
    return tx.txDataUnion.btcTxData?.amount.toString() ?? ''
  }

  if (isZCashTransaction(tx)) {
    return tx.txDataUnion.zecTxData?.amount.toString() ?? ''
  }

  assertNotReached('Unknown transaction type')
}

interface GetTransactionTransferredValueArgs {
  tx: TransactionInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
  txAccount: BraveWallet.AccountInfo | undefined
  txNetwork: BraveWallet.NetworkInfo | undefined
}

export function getTransactionTransferredValue(
  args: GetTransactionTransferredValueArgs
): {
  wei: Amount
  normalized: Amount
} {
  const { tx, txAccount, txNetwork, token } = args

  // Can't compute value with network decimals if no network or no account was
  // provided
  if (!txAccount || !txNetwork) {
    return {
      normalized: Amount.empty(),
      wei: Amount.empty()
    }
  }

  // ERC20 Approvals
  // ERC20 Transfers
  if (
    tx.txType === BraveWallet.TransactionType.ERC20Approve ||
    tx.txType === BraveWallet.TransactionType.ERC20Transfer
  ) {
    const [, amount] = tx.txArgs
    const wei = new Amount(amount)
    return {
      wei,
      normalized: wei.divideByDecimals(token?.decimals ?? 18)
    }
  }

  // ERC721
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // can only send 1 ERC721 NFT at a time
    return {
      wei: new Amount('1').multiplyByDecimals(18),
      normalized: new Amount('1')
    }
  }

  // ETH Swap
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    const { sellAmountWei, sellToken } = getETHSwapTransactionBuyAndSellTokens({
      tx,
      tokensList: []
    })
    const wei = sellAmountWei
    return {
      wei: sellAmountWei,
      normalized: sellToken
        ? wei.divideByDecimals(sellToken.decimals)
        : Amount.empty()
    }
  }

  // Solana Dapp Transactions
  if (isSolanaDappTransaction(tx)) {
    const lamportsMovedFromInstructions = getLamportsMovedFromInstructions(
      getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData) || [],
      tx.fromAddress ?? ''
    )

    const transferredValue = new Amount(getTransactionBaseValue(tx)).plus(
      lamportsMovedFromInstructions
    )

    return {
      wei: transferredValue,
      normalized: transferredValue.divideByDecimals(txNetwork.decimals)
    }
  }

  // Solana SPL
  if (isSolanaSplTransaction(tx)) {
    const wei = new Amount(getTransactionBaseValue(tx))
    return {
      wei,
      normalized: wei.divideByDecimals(token?.decimals ?? 9)
    }
  }

  // Solana
  // Filecoin Txs
  // to.toLowerCase() === SwapExchangeProxy:
  // ETHSend
  // SolanaSystemTransfer
  // Other
  const wei = new Amount(getTransactionBaseValue(tx))
  return {
    wei: wei,
    normalized: wei.divideByDecimals(txNetwork.decimals)
  }
}

export function getFormattedTransactionTransferredValue(
  args: GetTransactionTransferredValueArgs
): {
  normalizedTransferredValue: string
  normalizedTransferredValueExact: string
  weiTransferredValue: string
} {
  const { normalized, wei } = getTransactionTransferredValue(args)
  return {
    normalizedTransferredValue: normalized.format(6),
    normalizedTransferredValueExact: normalized.format(),
    weiTransferredValue: wei.value?.toString() || ''
  }
}

export function getTransactionGasLimit(transaction: TransactionInfo) {
  assert(
    isEthereumTransaction(transaction) || isFilecoinTransaction(transaction)
  )

  return isFilecoinTransaction(transaction)
    ? transaction.txDataUnion.filTxData.gasLimit
    : transaction.txDataUnion.ethTxData1559?.baseData.gasLimit || ''
}

/** For EVM and FIL transactions only */
export const getTransactionGas = (
  transaction: TransactionInfo
): {
  gasPrice: string
  maxFeePerGas: string
  maxPriorityFeePerGas: string
} => {
  if (isFilecoinTransaction(transaction)) {
    const { filTxData } = transaction.txDataUnion
    return {
      maxFeePerGas: filTxData.gasFeeCap || '',
      maxPriorityFeePerGas: filTxData.gasPremium,
      // baseFee = gasFeeCap - gasPremium
      gasPrice:
        new Amount(filTxData.gasFeeCap)
          .minus(filTxData.gasPremium)
          .value?.toString() || ''
    }
  }

  const { ethTxData1559 } = transaction.txDataUnion
  return {
    gasPrice: ethTxData1559?.baseData.gasPrice || '',
    maxFeePerGas: ethTxData1559?.maxFeePerGas || '',
    maxPriorityFeePerGas: ethTxData1559?.maxPriorityFeePerGas || ''
  }
}

export const isEIP1559Transaction = (transaction: TransactionInfo) => {
  if (
    !isEthereumTransaction(transaction) &&
    !isFilecoinTransaction(transaction)
  ) {
    return false
  }

  const { maxFeePerGas, maxPriorityFeePerGas } = getTransactionGas(transaction)
  return maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
}

/**
 * @param transaction the transaction to check
 * @param solFeeEstimates [FIXME] - Extract actual fees used in the Solana
 * transaction, instead of populating current estimates.
 * @returns string value of the gas fee
 */
export const getTransactionGasFee = (transaction: TransactionInfo): string => {
  assert(
    isEthereumTransaction(transaction) ||
      isFilecoinTransaction(transaction) ||
      isBitcoinTransaction(transaction) ||
      isZCashTransaction(transaction)
  )

  if (isBitcoinTransaction(transaction)) {
    return transaction.txDataUnion.btcTxData?.fee.toString() || ''
  }

  if (isZCashTransaction(transaction)) {
    return transaction.txDataUnion.zecTxData?.fee.toString() || ''
  }

  const { maxFeePerGas, gasPrice } = getTransactionGas(transaction)
  const gasLimit = getTransactionGasLimit(transaction)

  if (isEIP1559Transaction(transaction)) {
    return new Amount(maxFeePerGas).times(gasLimit).format()
  }

  return new Amount(gasPrice).times(gasLimit).format()
}

/**
 * Checks if a given transaction has an empty or zero-value gas limit
 *
 * @param tx - The transaction to check for a missing gas limit
 * @returns `true`, if the gas limit is missing, `false` otherwise
 */
export const isTransactionGasLimitMissing = (tx: TransactionInfo): boolean => {
  if (isSolanaTransaction(tx)) {
    return false
  }

  if (isBitcoinTransaction(tx)) {
    return false
  }

  if (isZCashTransaction(tx)) {
    return false
  }

  if (isEthereumTransaction(tx) || isFilecoinTransaction(tx)) {
    const gasLimit = getTransactionGasLimit(tx)
    return gasLimit === '' || Amount.normalize(gasLimit) === '0'
  }

  assertNotReached('Unknown transaction type')
}

export const parseTransactionFeesWithoutPrices = (tx: TransactionInfo) => {
  if (
    isSolanaTransaction(tx) ||
    isBitcoinTransaction(tx) ||
    isZCashTransaction(tx)
  ) {
    return {
      gasLimit: '',
      gasPrice: '',
      maxFeePerGas: '',
      maxPriorityFeePerGas: '',
      isEIP1559Transaction: false,
      isMissingGasLimit: false,
      gasPremium: '',
      gasFeeCap: ''
    }
  }

  if (isEthereumTransaction(tx) || isFilecoinTransaction(tx)) {
    const gasLimit = getTransactionGasLimit(tx)
    const { gasPrice, maxFeePerGas, maxPriorityFeePerGas } =
      getTransactionGas(tx)

    return {
      gasLimit: Amount.normalize(gasLimit),
      gasPrice: Amount.normalize(gasPrice),
      maxFeePerGas: Amount.normalize(maxFeePerGas),
      maxPriorityFeePerGas: Amount.normalize(maxPriorityFeePerGas),
      isEIP1559Transaction: isEIP1559Transaction(tx),
      isMissingGasLimit: isTransactionGasLimitMissing(tx),
      gasPremium: isFilecoinTransaction(tx)
        ? new Amount(tx.txDataUnion.filTxData.gasPremium).format()
        : '',
      gasFeeCap: isFilecoinTransaction(tx)
        ? new Amount(tx.txDataUnion.filTxData.gasFeeCap).format()
        : ''
    }
  }

  assertNotReached('Unknown transaction type')
}

export const getTransactionApprovalTargetAddress = (
  tx: TransactionInfo
): string => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [spender] = tx.txArgs // (address spender, uint256 amount)
    return spender
  }

  return ''
}

export function getTransactionDecimals({
  tx,
  erc721Token,
  network,
  sellToken,
  token
}: {
  tx: TransactionInfo
  network?: BraveWallet.NetworkInfo
  sellToken?: BraveWallet.BlockchainToken
  erc721Token?: BraveWallet.BlockchainToken
  token?: BraveWallet.BlockchainToken
}) {
  if (sellToken) {
    return sellToken.decimals
  }

  if (erc721Token) {
    return 0
  }

  if (token) {
    return token.decimals
  }

  if (network) {
    return network.decimals
  }

  switch (getCoinFromTxDataUnion(tx.txDataUnion)) {
    case BraveWallet.CoinType.SOL:
      return 9
    case BraveWallet.CoinType.ETH:
      return 18
    case BraveWallet.CoinType.FIL:
      return 18
    default:
      return 18
  }
}

export const getTransactionErc721TokenId = (
  tx: TransactionInfo
): string | undefined => {
  if (
    [
      BraveWallet.TransactionType.ERC721TransferFrom,
      BraveWallet.TransactionType.ERC721SafeTransferFrom
    ].includes(tx.txType)
  ) {
    // (address owner, address to, uint256 tokenId)
    const [, , tokenID] = tx.txArgs
    return tokenID && `#${Amount.normalize(tokenID)}`
  }
  return undefined
}

/**
 * Checks if a given address is a known contract address from our token
 * registry.
 *
 * @remarks
 *
 * This function must only be used for the following transaction types:
 *  - ERC20Transfer
 *  - ERC721TransferFrom
 *  - ERC721SafeTransferFrom
 *  - SolanaSPLTokenTransfer
 *  - SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
 *
 * @param address - The address to check
 * @returns false if case no error, true otherwise
 */
function isKnownTokenContractAddress(
  address: string,
  tokenList: BraveWallet.BlockchainToken[]
) {
  return tokenList?.some(
    (token) => token.contractAddress.toLowerCase() === address.toLowerCase()
  )
}

/**
 * Checks if a given transaction is sending funds to a known contract address
 * from our token registry.
 *
 * @param tokenList - A list of Erc & SPL tokens to check against
 * @param tx - The transaction to check
 * @returns `true` if the to address is a known erc & SPL token contract
 * address, `false` otherwise
 */
export const isSendingToKnownTokenContractAddress = (
  tx: Pick<TransactionInfo, 'txType' | 'txArgs' | 'txDataUnion'>,
  tokenList: BraveWallet.BlockchainToken[]
): boolean => {
  // ERC20Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [recipient] = tx.txArgs // [address recipient, uint256 amount]
    const contractAddressError = isKnownTokenContractAddress(
      recipient,
      tokenList
    )
    return contractAddressError
  }

  // ERC721TransferFrom
  // ERC721SafeTransferFrom
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the caller
    // (fromAddress).
    // [address owner, address to, uint256 tokenId]
    const [, toAddress] = tx.txArgs
    const contractAddressError = isKnownTokenContractAddress(
      toAddress,
      tokenList
    )
    return contractAddressError
  }

  // ERC20Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return false
  }

  // Solana SPL Token Transfer
  if (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType ===
      BraveWallet.TransactionType
        .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  ) {
    const contractAddressError = isKnownTokenContractAddress(
      getTransactionInteractionAddress(tx) ?? '',
      tokenList
    )
    return contractAddressError
  }

  return false
}

/**
 * Checks if a given transaction's sender and recipient addresses
 * are the same.
 *
 * @param tx - The transaction to check
 */
export const transactionHasSameAddressError = (
  tx: TransactionInfo
): boolean => {
  const { txArgs, txType, fromAddress: from = '' } = tx

  // transfer(address recipient, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [recipient] = txArgs // (address recipient, uint256 amount)
    return recipient.toLowerCase() === from.toLowerCase()
  }

  // transferFrom(address owner, address to, uint256 tokenId)
  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the caller
    // (fromAddress).
    // (address owner, address to, uint256 tokenId)
    const [owner, toAddress] = txArgs
    return toAddress.toLowerCase() === owner.toLowerCase()
  }

  // approve(address spender, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    const [spender] = txArgs // (address spender, uint256 amount)
    return spender.toLowerCase() === from.toLowerCase()
  }

  if (isSolanaSplTransaction(tx)) {
    return (
      (tx.txDataUnion.solanaTxData.toWalletAddress ?? '').toLowerCase() ===
      from.toLowerCase()
    )
  }

  if (
    BraveWallet.TransactionType.ETHSend ||
    BraveWallet.TransactionType.ETHSwap ||
    BraveWallet.TransactionType.Other
  ) {
    return false
  }

  // unknown
  return getTransactionToAddress(tx).toLowerCase() === from.toLowerCase()
}

export function getGasFeeFiatValue({
  gasFee,
  networkSpotPrice,
  txNetwork
}: {
  gasFee: string
  networkSpotPrice: string
  txNetwork?: Pick<BraveWallet.NetworkInfo, 'decimals'>
}) {
  if (!txNetwork || !networkSpotPrice) {
    return ''
  }

  return new Amount(gasFee)
    .divideByDecimals(txNetwork.decimals)
    .times(networkSpotPrice)
    .formatAsFiat()
}

export const accountHasInsufficientFundsForTransaction = ({
  accountNativeBalance,
  accountTokenBalance,
  gasFee,
  tx,
  sellAmountWei = new Amount('0'),
  sellTokenBalance
}: {
  accountNativeBalance: string
  accountTokenBalance: string
  gasFee: string
  tx: TransactionInfo
  sellAmountWei: Amount
  sellTokenBalance: string
}): boolean => {
  const { txType, txArgs } = tx

  if (isSolanaDappTransaction(tx)) {
    const lamportsMovedFromInstructions = getLamportsMovedFromInstructions(
      getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData) || [],
      tx.fromAddress || ''
    )

    const transferredValue = new Amount(getTransactionBaseValue(tx)).plus(
      lamportsMovedFromInstructions
    )

    return (
      accountNativeBalance !== '' &&
      transferredValue.plus(gasFee).gt(accountNativeBalance)
    )
  }

  // ERC20
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    return false // can approve for more tokens than you own
  }

  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = txArgs // (address recipient, uint256 amount)
    return (
      accountTokenBalance !== '' && new Amount(amount).gt(accountTokenBalance)
    )
  }

  // ERC721
  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    return false
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    return (
      accountTokenBalance !== '' &&
      new Amount(getTransactionBaseValue(tx)).gt(accountTokenBalance)
    )
  }

  // Eth Swap
  if (txType === BraveWallet.TransactionType.ETHSwap) {
    return sellTokenBalance !== '' && sellAmountWei.gt(sellTokenBalance)
  }
  // ETHSend
  // SolanaSystemTransfer
  // Other
  return (
    accountNativeBalance !== '' &&
    new Amount(getTransactionBaseValue(tx))
      .plus(gasFee)
      .gt(accountNativeBalance)
  )
}

export function getTransactionTransferredToken({
  tx,
  txNetwork,
  sellToken,
  token
}: {
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
}): BraveWallet.BlockchainToken | undefined {
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    return sellToken
  }

  if (
    tx.txType === BraveWallet.TransactionType.ERC20Approve ||
    tx.txType === BraveWallet.TransactionType.ERC20Transfer ||
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom ||
    isSolanaSplTransaction(tx)
  ) {
    return token
  }

  // default
  const nativeAsset = txNetwork ? makeNetworkAsset(txNetwork) : undefined
  return nativeAsset
}

export function getTransactionTokenSymbol({
  tx,
  txNetwork,
  sellToken,
  token
}: {
  tx: TransactionInfo
  txNetwork?: Pick<BraveWallet.NetworkInfo, 'symbol'>
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
}): string {
  if (isSolanaDappTransaction(tx)) {
    return txNetwork?.symbol ?? ''
  }

  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    return sellToken?.symbol || ''
  }

  if (
    tx.txType === BraveWallet.TransactionType.ERC20Approve ||
    tx.txType === BraveWallet.TransactionType.ERC20Transfer ||
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom ||
    isSolanaSplTransaction(tx)
  ) {
    return token?.symbol || ''
  }

  return txNetwork?.symbol || ''
}

export const getTransactionIntent = ({
  buyAmount,
  buyToken,
  erc721TokenId,
  normalizedTransferredValue,
  sellAmount,
  sellToken,
  token,
  transactionNetwork,
  tx
}: {
  buyAmount?: Amount
  buyToken?: BraveWallet.BlockchainToken
  erc721TokenId?: string
  normalizedTransferredValue: string
  sellAmount?: Amount
  sellToken?: BraveWallet.BlockchainToken
  token?: BraveWallet.BlockchainToken
  transactionNetwork?: BraveWallet.NetworkInfo
  tx: TransactionInfo
}): string => {
  // ERC20 Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return (
      toProperCase(getLocale('braveWalletApprovalTransactionIntent')) +
        ' ' +
        token?.symbol ?? ''
    )
  }

  // ERC20 Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    return getLocale('braveWalletTransactionIntentSend').replace(
      '$1',
      new Amount(normalizedTransferredValue).formatAsAsset(6, token?.symbol)
    )
  }

  // ERC 721
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    return getLocale('braveWalletTransactionIntentSend').replace(
      '$1',
      `${token?.symbol ?? ''} ${erc721TokenId}`
    )
  }

  // Solana Dapps
  if (isSolanaDappTransaction(tx)) {
    return tx.txType === BraveWallet.TransactionType.SolanaSwap
      ? getLocale('braveWalletSwap')
      : getLocale('braveWalletTransactionIntentDappInteraction')
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    return getLocale('braveWalletTransactionIntentSend').replace(
      '$1',
      new Amount(normalizedTransferredValue).formatAsAsset(6, token?.symbol)
    )
  }

  // ETHSwap
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    return getLocale('braveWalletTransactionIntentSwap')
      .replace('$1', sellAmount?.formatAsAsset(6, sellToken?.symbol) ?? '')
      .replace('$2', buyAmount?.formatAsAsset(6, buyToken?.symbol) ?? '')
  }

  // default / other
  return getLocale('braveWalletTransactionIntentSend').replace(
    '$1',
    new Amount(normalizedTransferredValue).formatAsAsset(
      6,
      transactionNetwork?.symbol
    )
  )
}

export const accountHasInsufficientFundsForGas = ({
  accountNativeBalance,
  gasFee
}: {
  accountNativeBalance: string
  gasFee: string
}): boolean => {
  return (
    accountNativeBalance !== '' && new Amount(gasFee).gt(accountNativeBalance)
  )
}

export const getIsTxApprovalUnlimited = (tx: TransactionInfo): boolean => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [, amount] = tx.txArgs // (address spender, uint256 amount)
    return new Amount(amount).eq(MAX_UINT256)
  }

  return false
}

export const getIsRevokeApprovalTx = (tx: TransactionInfo): boolean => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [, amount] = tx.txArgs // (address spender, uint256 amount)
    return new Amount(amount).toNumber() <= 0
  }

  return false
}

export const isSwapTransaction = (tx: TransactionInfo) => {
  if (getTransactionToAddress(tx).toLowerCase() === SwapExchangeProxy) {
    return true
  }

  return [
    BraveWallet.TransactionType.ETHSwap,
    BraveWallet.TransactionType.SolanaSwap
  ].includes(tx.txType)
}

export const isBridgeTransaction = (tx: TransactionInfo) => {
  return tx.swapInfo?.fromChainId !== tx.swapInfo?.toChainId
}

export const getTransactionFormattedSendCurrencyTotal = ({
  normalizedTransferredValue,
  sellToken,
  token,
  tx,
  txNetwork
}: {
  normalizedTransferredValue: string
  sellToken?: BraveWallet.BlockchainToken
  token?: BraveWallet.BlockchainToken
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
}): string => {
  const sendToken = getTransactionTransferredToken({
    tx,
    sellToken,
    token,
    txNetwork
  })
  return new Amount(normalizedTransferredValue).formatAsAsset(
    6,
    sendToken?.symbol
  )
}

export const getTransactionFiatValues = ({
  gasFee,
  networkSpotPrice,
  normalizedTransferredValue,
  sellAmountWei,
  sellToken,
  spotPriceRegistry,
  token,
  transferredValueWei,
  tx,
  txNetwork
}: {
  gasFee: string
  networkSpotPrice: string
  normalizedTransferredValue: string
  sellAmountWei?: string
  sellToken?: BraveWallet.BlockchainToken
  spotPriceRegistry: SpotPriceRegistry
  token?: BraveWallet.BlockchainToken
  transferredValueWei?: string
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
}): {
  fiatValue: string
  fiatTotal: string
  gasFeeFiat: string
} => {
  const gasFeeFiat = getGasFeeFiatValue({
    gasFee,
    networkSpotPrice,
    txNetwork
  })

  // Solana Dapps
  if (isSolanaDappTransaction(tx)) {
    const transferredAmountFiat = txNetwork
      ? computeFiatAmount({
          spotPriceRegistry,
          value: transferredValueWei || '',
          token: {
            decimals: txNetwork.decimals,
            symbol: txNetwork.symbol,
            contractAddress: '',
            chainId: txNetwork.chainId,
            coin: txNetwork.coin,
            coingeckoId: ''
          }
        })
      : Amount.empty()

    const totalAmountFiat = new Amount(gasFeeFiat).plus(transferredAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: transferredAmountFiat.toNumber().toString(),
      fiatTotal: totalAmountFiat.toNumber().toString()
    }
  }

  // ERC20 Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = tx.txArgs // (address recipient, uint256 amount) → bool

    const price = token
      ? getTokenPriceAmountFromRegistry(spotPriceRegistry, token)
      : Amount.empty()

    const sendAmountFiat = new Amount(amount)
      .divideByDecimals(token?.decimals ?? 18)
      .times(price)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat.toNumber().toString(),
      fiatTotal: new Amount(gasFeeFiat)
        .plus(sendAmountFiat)
        .toNumber()
        .toString()
    }
  }

  // ERC721 TransferFrom
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the
    // caller (fromAddress).
    const totalAmountFiat = gasFeeFiat

    return {
      gasFeeFiat,
      fiatValue: '0', // Display NFT values in the future
      fiatTotal: new Amount(totalAmountFiat).toNumber().toString()
    }
  }

  // ERC20 Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return {
      gasFeeFiat,
      fiatValue: '0',
      fiatTotal: gasFeeFiat
    }
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    const price = token
      ? getTokenPriceAmountFromRegistry(spotPriceRegistry, token)
      : Amount.empty()
    const sendAmountFiat = new Amount(normalizedTransferredValue).times(price)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat.toNumber().toString(),
      fiatTotal: new Amount(gasFeeFiat)
        .plus(sendAmountFiat)
        .toNumber()
        .toString()
    }
  }

  // ETH SWAP
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    const sellAmountFiat =
      sellToken && sellAmountWei
        ? computeFiatAmount({
            spotPriceRegistry,
            value: sellAmountWei,
            token: sellToken
          })
        : Amount.empty()

    const totalAmountFiat = new Amount(gasFeeFiat).plus(sellAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sellAmountFiat.toNumber().toString(),
      fiatTotal: totalAmountFiat.toNumber().toString()
    }
  }

  // DEFAULT
  const sendAmountFiat = txNetwork
    ? computeFiatAmount({
        spotPriceRegistry,
        value: getTransactionBaseValue(tx) || '',
        token: {
          decimals: txNetwork.decimals,
          symbol: txNetwork.symbol,
          contractAddress: '',
          chainId: txNetwork.chainId,
          coin: txNetwork.coin,
          coingeckoId: ''
        }
      })
    : Amount.empty()

  return {
    gasFeeFiat,
    fiatValue: sendAmountFiat.toNumber().toString(),
    fiatTotal: new Amount(gasFeeFiat).plus(sendAmountFiat).toNumber().toString()
  }
}

export const parseTransactionWithoutPrices = ({
  accounts,
  tx,
  transactionAccount,
  transactionNetwork,
  tokensList
}: {
  accounts: EntityState<BraveWallet.AccountInfo>
  tx: TransactionInfo
  transactionAccount: BraveWallet.AccountInfo
  transactionNetwork: BraveWallet.NetworkInfo
  tokensList: BraveWallet.BlockchainToken[]
}): ParsedTransactionWithoutFiatValues => {
  const to = getTransactionToAddress(tx)

  const nativeAsset = makeNetworkAsset(transactionNetwork)
  const token = findTransactionToken(tx, tokensList)
  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount,
    sellAmountWei,
    buyAmountWei
  } = getETHSwapTransactionBuyAndSellTokens({
    nativeAsset,
    tokensList,
    tx
  })

  const {
    normalizedTransferredValue,
    normalizedTransferredValueExact,
    weiTransferredValue
  } = getFormattedTransactionTransferredValue({
    tx,
    txAccount: transactionAccount,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const erc721BlockchainToken = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType)
    ? token
    : undefined

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  const {
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas,
    isEIP1559Transaction
  } = parseTransactionFeesWithoutPrices(tx)

  const erc721TokenId = getTransactionErc721TokenId(tx)

  const missingGasLimitError = isMissingGasLimit
    ? getLocale('braveWalletMissingGasLimitError')
    : undefined

  const approvalTargetLabel = getAddressLabel(approvalTarget, accounts)
  const coinType = getCoinFromTxDataUnion(tx.txDataUnion)
  const createdTime = makeSerializableTimeDelta(tx.createdTime)

  const contractAddressError = isSendingToKnownTokenContractAddress(
    tx,
    tokensList
  )
    ? getLocale('braveWalletContractAddressError')
    : undefined

  const decimals = getTransactionDecimals({
    tx,
    network: transactionNetwork,
    sellToken,
    erc721Token: erc721BlockchainToken,
    token
  })

  const instructions = getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData)

  const sameAddressError = transactionHasSameAddressError(tx)
    ? getLocale('braveWalletSameAddressError')
    : undefined

  const symbol = getTransactionTokenSymbol({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const intent = getTransactionIntent({
    normalizedTransferredValue,
    tx,
    buyAmount,
    buyToken,
    erc721TokenId,
    sellAmount,
    sellToken,
    token,
    transactionNetwork
  })

  const isSendingToZeroXExchangeProxy =
    tx.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() ===
    SwapExchangeProxy

  const formattedSendCurrencyTotal = getTransactionFormattedSendCurrencyTotal({
    normalizedTransferredValue,
    sellToken,
    token,
    txNetwork: transactionNetwork,
    tx
  })

  return {
    approvalTarget,
    approvalTargetLabel,
    chainId: transactionNetwork?.chainId || '',
    coinType,
    contractAddressError,
    createdTime,
    decimals,
    erc721BlockchainToken,
    erc721TokenId,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    hash: tx.txHash,
    id: tx.id,
    instructions,
    intent,
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSendingToZeroXExchangeProxy,
    isSolanaDappTransaction: isSolanaDappTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSwap: isSwapTransaction(tx),
    maxFeePerGas,
    maxPriorityFeePerGas,
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    missingGasLimitError,
    nonce: getTransactionNonce(tx),
    originInfo: tx.originInfo,
    recipient: to,
    recipientLabel: getAddressLabel(to, accounts),
    sameAddressError,
    sellAmount,
    sellAmountWei,
    sellToken,
    senderLabel: getAccountLabel(tx.fromAccountId, accounts),
    status: tx.txStatus,
    symbol,
    token,
    txType: tx.txType,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact,
    weiTransferredValue,
    formattedSendCurrencyTotal,
    isAssociatedTokenAccountCreation: isAssociatedTokenAccountCreationTx(tx)
  }
}

export const parseTransactionWithPrices = ({
  accounts,
  tx,
  transactionAccount,
  transactionNetwork,
  spotPriceRegistry,
  gasFee,
  tokensList
}: {
  accounts: EntityState<BraveWallet.AccountInfo>
  tx: TransactionInfo
  transactionAccount: BraveWallet.AccountInfo
  transactionNetwork: BraveWallet.NetworkInfo
  tokensList: BraveWallet.BlockchainToken[]
  spotPriceRegistry: SpotPriceRegistry
  gasFee: string
}): ParsedTransaction => {
  const networkSpotPrice = transactionNetwork
    ? getTokenPriceAmountFromRegistry(spotPriceRegistry, {
        symbol: transactionNetwork.symbol,
        contractAddress: '',
        chainId: transactionNetwork.chainId,
        coingeckoId: ''
      }).format()
    : ''

  const {
    token,
    sellToken,
    weiTransferredValue,
    value: normalizedTransferredValue,
    sellAmountWei,
    ...txBase
  } = parseTransactionWithoutPrices({
    accounts,
    transactionAccount,
    transactionNetwork,
    tx,
    tokensList
  })

  return {
    token,
    sellToken,
    sellAmountWei,
    weiTransferredValue,
    value: normalizedTransferredValue,
    ...txBase,
    ...getTransactionFiatValues({
      gasFee,
      sellAmountWei: sellAmountWei?.format(),
      networkSpotPrice,
      normalizedTransferredValue,
      spotPriceRegistry,
      tx,
      sellToken,
      token,
      txNetwork: transactionNetwork,
      transferredValueWei: weiTransferredValue
    })
  }
}

export function toTxDataUnion<D extends keyof BraveWallet.TxDataUnion>(
  unionItem: Pick<BraveWallet.TxDataUnion, D>
) {
  return Object.assign({}, unionItem) as BraveWallet.TxDataUnion
}

export function isSolanaSignTransactionRequest(
  request?:
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
): request is BraveWallet.SignTransactionRequest {
  // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
  return !!(request as BraveWallet.SignTransactionRequest | undefined)?.txData
    ?.solanaTxData
}

export function isSolanaSignAllTransactionsRequest(
  request?:
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
): request is BraveWallet.SignAllTransactionsRequest {
  // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
  return !!(request as BraveWallet.SignAllTransactionsRequest | undefined)
    ?.txDatas
}

export function getTxDataFromSolSignTxRequest(
  selectedQueueData: BraveWallet.SignTransactionRequest
): BraveWallet.SolanaTxData | undefined {
  return selectedQueueData.txData?.solanaTxData
}

export function getTxDatasFromSolSignAllTxsRequest(
  selectedQueueData: BraveWallet.SignAllTransactionsRequest
): BraveWallet.SolanaTxData[] {
  return selectedQueueData.txDatas
    .map(({ solanaTxData }) => solanaTxData)
    .filter((data): data is BraveWallet.SolanaTxData => !!data)
}

export function getTxDatasFromQueuedSolSignRequest(
  selectedQueueData:
    | BraveWallet.SignAllTransactionsRequest
    | BraveWallet.SignTransactionRequest
): BraveWallet.SolanaTxData[] {
  if (isSolanaSignAllTransactionsRequest(selectedQueueData)) {
    return getTxDatasFromSolSignAllTxsRequest(selectedQueueData)
  }

  if (isSolanaSignTransactionRequest(selectedQueueData)) {
    const txData = getTxDataFromSolSignTxRequest(selectedQueueData)
    return txData ? [txData] : []
  }

  return []
}

export const isAssociatedTokenAccountCreationTx = (
  tx: Pick<BraveWallet.TransactionInfo, 'txType'> | undefined
) =>
  tx?.txType ===
  BraveWallet.TransactionType
    .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation

export function getTransactionTypeName(txType: BraveWallet.TransactionType) {
  switch (txType) {
    case BraveWallet.TransactionType.ERC1155SafeTransferFrom:
      return getLocale('braveWalletTransactionTypeNameSafeTransferFrom')

    case BraveWallet.TransactionType.ERC20Approve:
      return getLocale('braveWalletTransactionTypeNameErc20Approve')

    case BraveWallet.TransactionType.ERC20Transfer:
      return getLocale('braveWalletTransactionTypeNameTokenTransfer')

    case BraveWallet.TransactionType.ERC721SafeTransferFrom:
      return getLocale('braveWalletTransactionTypeNameSafeTransferFrom')

    case BraveWallet.TransactionType.ERC721TransferFrom:
      return getLocale('braveWalletTransactionTypeNameNftTransfer')

    case BraveWallet.TransactionType.ETHFilForwarderTransfer:
      return getLocale('braveWalletTransactionTypeNameForwardFil')

    case BraveWallet.TransactionType.ETHSend:
      return getLocale('braveWalletTransactionIntentSend').replace('$1', 'ETH')

    case BraveWallet.TransactionType.ETHSwap:
      return getLocale('braveWalletSwap')

    case BraveWallet.TransactionType.Other:
      return getLocale('braveWalletTransactionTypeNameOther')

    case BraveWallet.TransactionType.SolanaCompressedNftTransfer:
      return getLocale('braveWalletTransactionTypeNameCompressedNftTransfer')

    case BraveWallet.TransactionType.SolanaDappSignAndSendTransaction:
      return getLocale(
        'braveWalletTransactionTypeNameSignAndSendDappTransaction'
      )

    case BraveWallet.TransactionType.SolanaDappSignTransaction:
      return getLocale('braveWalletTransactionTypeNameSignDappTransaction')

    case BraveWallet.TransactionType.SolanaSPLTokenTransfer:
      return getLocale('braveWalletTransactionTypeNameTokenTransfer')

    case BraveWallet.TransactionType
      .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation:
      return getLocale(
        'braveWalletTransactionTypeNameSplTokenTransfer' +
          'WithAssociatedTokenAccountCreation'
      )

    case BraveWallet.TransactionType.SolanaSwap:
      return getLocale('braveWalletSwap')

    case BraveWallet.TransactionType.SolanaSystemTransfer:
      return getLocale('braveWalletTransactionIntentSend').replace('$1', 'SOL')

    default:
      return getLocale('braveWalletTransactionTypeNameOther')
  }
}
