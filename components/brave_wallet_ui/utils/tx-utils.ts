// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// types
import { BraveWallet, SolFeeEstimates, WalletAccountType } from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'

// constants
import { MAX_UINT256, NATIVE_ASSET_CONTRACT_ADDRESS_0X } from '../common/constants/magics'
import { SwapExchangeProxy } from '../common/hooks/address-labels'

// utils
import { getLocale } from '../../common/locale'
import Amount from './amount'
import {
  getTypedSolanaTxInstructions,
  SolanaParamsWithLamports,
  TypedSolanaInstructionWithParams
} from './solana-instruction-utils'
import { getBalance } from './balance-utils'
import { toProperCase } from './string-utils'
import { makeNetworkAsset } from '../options/asset-options'
import { findTokenByContractAddress } from './asset-utils'
import { getNetworkFromTXDataUnion } from './network-utils'
import { ParsedTransaction } from './transaction-parser'
import { computeFiatAmount, findAssetPrice } from './pricing-utils'

type Order = 'ascending' | 'descending'

type EIP1559TransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    ethTxData1559: BraveWallet.TxData1559
  }
}

type SolanaTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    solanaTxData: BraveWallet.SolanaTxData
  }
}

type FileCoinTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    filTxData: BraveWallet.FilTxData
  }
}

type TransactionInfoWithNetworkInfo = BraveWallet.TransactionInfo & Pick<ParsedTransaction, 'network'>
type TransactionInfoWithAccountAddress = BraveWallet.TransactionInfo & Pick<ParsedTransaction, 'accountAddress'>

export const sortTransactionByDate = (transactions: BraveWallet.TransactionInfo[], order: Order = 'ascending') => {
  return [...transactions].sort(function (x: BraveWallet.TransactionInfo, y: BraveWallet.TransactionInfo) {
    return order === 'ascending'
      ? Number(x.createdTime.microseconds) - Number(y.createdTime.microseconds)
      : Number(y.createdTime.microseconds) - Number(x.createdTime.microseconds)
  })
}

export const getTransactionStatusString = (statusId: number) => {
  switch (statusId) {
    case BraveWallet.TransactionStatus.Unapproved:
      return getLocale('braveWalletTransactionStatusUnapproved')
    case BraveWallet.TransactionStatus.Approved:
      return getLocale('braveWalletTransactionStatusApproved')
    case BraveWallet.TransactionStatus.Rejected:
      return getLocale('braveWalletTransactionStatusRejected')
    case BraveWallet.TransactionStatus.Submitted:
      return getLocale('braveWalletTransactionStatusSubmitted')
    case BraveWallet.TransactionStatus.Confirmed:
      return getLocale('braveWalletTransactionStatusConfirmed')
    case BraveWallet.TransactionStatus.Error:
      return getLocale('braveWalletTransactionStatusError')
    case BraveWallet.TransactionStatus.Dropped:
      return getLocale('braveWalletTransactionStatusDropped')
    default:
      return ''
  }
}

export function isSolanaTransaction (transaction: BraveWallet.TransactionInfo): transaction is SolanaTransactionInfo {
  const { txType, txDataUnion: { solanaTxData } } = transaction
  return SolanaTransactionTypes.includes(txType) ||
    (txType === BraveWallet.TransactionType.Other && solanaTxData !== undefined)
}

export function isSolanaSplTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
  return (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  )
}

export function isSolanaDappTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
  return (
    tx.txDataUnion.solanaTxData !== undefined &&
    [
      BraveWallet.TransactionType.SolanaDappSignTransaction,
      BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
      BraveWallet.TransactionType.SolanaSwap
    ].includes(tx.txType)
  )
}

export function isFilecoinTransaction (tx: BraveWallet.TransactionInfo): tx is FileCoinTransactionInfo {
  return tx.txDataUnion.filTxData !== undefined
}

export function getTransactionGasLimit (transaction: BraveWallet.TransactionInfo) {
  return isFilecoinTransaction(transaction)
    ? transaction.txDataUnion.filTxData.gasLimit
    : transaction.txDataUnion.ethTxData1559?.baseData.gasLimit || ''
}

export function getTransactionGas (transaction: BraveWallet.TransactionInfo): { gasPrice: string, maxFeePerGas: any, maxPriorityFeePerGas: any } {
  return {
    gasPrice: transaction.txDataUnion.ethTxData1559?.baseData.gasPrice || '',
    maxFeePerGas: transaction.txDataUnion.ethTxData1559?.maxFeePerGas || '',
    maxPriorityFeePerGas: transaction.txDataUnion.ethTxData1559?.maxPriorityFeePerGas || ''
  }
}

export const isEIP1559Transaction = (transaction: BraveWallet.TransactionInfo): transaction is EIP1559TransactionInfo => {
  const { maxFeePerGas, maxPriorityFeePerGas } = getTransactionGas(transaction)
  return maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
}

export const getGasFee = (
  transaction: BraveWallet.TransactionInfo,
  solFeeEstimates: SolFeeEstimates | undefined
): string => {
  // [FIXME] - Extract actual fees used in the Solana transaction, instead of populating current estimates.
  const { maxFeePerGas, gasPrice } = getTransactionGas(transaction)
  const gasLimit = getTransactionGasLimit(transaction)

  if (isSolanaTransaction(transaction)) {
    return new Amount(solFeeEstimates?.fee.toString() ?? '')
      .format()
  }

  if (isEIP1559Transaction(transaction)) {
    return new Amount(maxFeePerGas)
      .times(gasLimit)
      .format()
  }

  return new Amount(gasPrice)
    .times(gasLimit)
    .format()
}

/**
 * Checks if a given address is a known contract address from our token registry.
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
 * @param fullTokenList - A list of Erc & SPL tokens to check against
 * @param to - The address to check
 * @returns `true` if the to address is a known erc & SPL token contract address, `false` otherwise
*/
export const isSendingToKnownTokenContractAddress = (
  tx: BraveWallet.TransactionInfo,
  to: string,
  fullTokenList: BraveWallet.BlockchainToken[]
): boolean => {
  const { txArgs, txType } = tx

  if (
    to === SwapExchangeProxy ||
    txType === BraveWallet.TransactionType.ERC20Approve ||
    txType === BraveWallet.TransactionType.ETHSend ||
    txType === BraveWallet.TransactionType.Other
  ) {
    return false
  }

  function checkForContractAddressError (to: string) {
    return fullTokenList?.some(token => token.contractAddress.toLowerCase() === to.toLowerCase())
  }

  // transfer(address recipient, uint256 amount) → bool
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [recipient] = txArgs // (address recipient, uint256 amount)
    return checkForContractAddressError(recipient)
  }

  // transferFrom(address owner, address to, uint256 tokenId)
  // safeTransferFrom(address owner, address to, uint256 tokenId)
  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the
    // caller (fromAddress).
    const [, toAddress] = txArgs // (address owner, address to, uint256 tokenId)
    return checkForContractAddressError(toAddress)
  }

  // Solana SPL Transfers
  if (isSolanaSplTransaction(tx)) {
    return checkForContractAddressError(tx.txDataUnion.solanaTxData.toWalletAddress ?? '')
  }

  // Other
  return checkForContractAddressError(to)
}

/**
 * Checks if a given set of sender and recipient addresses are the
 * same.
 *
 * @param to - The recipient address
 * @param from - The sender address
*/
export const transactionHasSameAddressError = (
  tx: BraveWallet.TransactionInfo
): boolean => {
  const { txArgs, txType, fromAddress: from } = tx

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
    const [owner, toAddress] = txArgs // (address owner, address to, uint256 tokenId)
    return toAddress.toLowerCase() === owner.toLowerCase()
  }

  // approve(address spender, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    const [spender] = txArgs // (address spender, uint256 amount)
    return spender.toLowerCase() === from.toLowerCase()
  }

  if (isSolanaSplTransaction(tx)) {
    return (tx.txDataUnion.solanaTxData.toWalletAddress ?? '')
      .toLowerCase() === from.toLowerCase()
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

/**
 * Checks if a given gasLimit is empty or zero-value, and returns an
 * appropriate localized error string.
 *
 * @remarks
 *
 * This function may only be used on ALL transaction types.
 *
 * @param gasLimit - The parsed gasLimit string.
 * @returns Localized string describing the error, or undefined in case of
 * no error.
*/
export const isMissingGasLimit = (gasLimit: string): boolean => {
  return (gasLimit === '' || Amount.normalize(gasLimit) === '0')
}

export function getTransactionTransferedValue ({
  tx,
  txNetwork,
  token,
  sellToken
}: {
  tx: BraveWallet.TransactionInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
  txNetwork: BraveWallet.NetworkInfo
}): {
  wei: Amount
  normalized: Amount
} {
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
    // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
    const [, sellAmountArg] = tx.txArgs
    const wei = new Amount(sellAmountArg || tx.txDataUnion.ethTxData1559?.baseData.value || '')
    return {
      wei,
      normalized: wei.divideByDecimals(sellToken?.decimals ?? txNetwork.decimals)
    }
  }

  // Solana Dapp Transactions
  if (isSolanaDappTransaction(tx)) {
    const baseValue = tx.txDataUnion.solanaTxData.amount.toString() ?? ''

    const lamportsMovedFromInstructions = getLamportsMovedFromInstructions(
      getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData) || [],
      tx.fromAddress
    )

    const transferedValue = new Amount(baseValue)
      .plus(lamportsMovedFromInstructions)

    return {
      wei: transferedValue,
      normalized: transferedValue.divideByDecimals(txNetwork.decimals)
    }
  }

  if (isSolanaSplTransaction(tx)) {
    const wei = new Amount(tx.txDataUnion.solanaTxData.amount.toString() ?? '')
    return {
      wei,
      normalized: wei.divideByDecimals(token?.decimals ?? 9)
    }
  }

  if (isSolanaTransaction(tx)) {
    const wei = new Amount(tx.txDataUnion.solanaTxData.lamports.toString() ?? '')
    return {
      wei,
      normalized: wei.divideByDecimals(token?.decimals ?? txNetwork.decimals)
    }
  }

  if (isFilecoinTransaction(tx)) {
    const wei = new Amount(tx.txDataUnion.filTxData.value || '')
    return {
      wei,
      normalized: wei.divideByDecimals(txNetwork.decimals)
    }
  }

  // Other
  const wei = new Amount(
    tx.txDataUnion.ethTxData1559?.baseData.value || // EVM (1559)
    tx.txDataUnion.ethTxData?.value || // EVM
    ''
  )
  return {
    wei: wei,
    normalized: wei.divideByDecimals(txNetwork.decimals)
  }
}

export function getLamportsMovedFromInstructions (
  instructions: TypedSolanaInstructionWithParams[],
  fromAddress: string
) {
  return instructions.reduce((acc, instruction) => {
    const lamportsAmount = (instruction.params as SolanaParamsWithLamports)?.lamports?.toString() ?? '0'

    switch (instruction.type) {
      case 'Transfer':
      case 'TransferWithSeed': {
        const { fromPubkey, toPubkey } = instruction.params

        // only show lamports as transfered if the amount is going to a different pubKey
        if (!toPubkey.equals(fromPubkey)) {
          return acc.plus(lamportsAmount)
        }
        return acc
      }

      case 'WithdrawNonceAccount': {
        const { noncePubkey, toPubkey } = instruction.params

        if (noncePubkey.equals(new Solana.PublicKey(fromAddress))) {
          return acc.plus(lamportsAmount)
        }

        if (toPubkey.equals(new Solana.PublicKey(fromAddress))) {
          return acc.minus(lamportsAmount)
        }

        return acc
      }

      case 'Create':
      case 'CreateWithSeed': {
        const { fromPubkey } = instruction.params

        if (fromPubkey.toString() === fromAddress) {
          return acc.plus(lamportsAmount)
        }

        return acc
      }

      default: return acc.plus(lamportsAmount)
    }
  }, new Amount(0)) ?? 0
}

export function getFormattedTransactionTransferedValue ({
  tx,
  txNetwork,
  token,
  sellToken
}: {
  tx: BraveWallet.TransactionInfo
  txNetwork: BraveWallet.NetworkInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken | undefined
}): {
  value: string
  valueExact: string
  weiValue: string
} {
  const { normalized: transferedValue, wei } = getTransactionTransferedValue({
    tx,
    txNetwork,
    token,
    sellToken
  })

  const value = transferedValue.format(6)
  const valueExact = transferedValue.format()
  return {
    value,
    valueExact,
    weiValue: wei.value?.toString() || ''
  }
}

export function findTransactionAccount <T extends WalletAccountType | BraveWallet.AccountInfo> (accounts: T[], transaction: BraveWallet.TransactionInfo): T | undefined {
  return accounts.find((account) => account.address.toLowerCase() === transaction.fromAddress.toLowerCase())
}

export function getTransactionInteractionAddress (tx: BraveWallet.TransactionInfo): string {
  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.toWalletAddress ?? ''
  }

  if (isFilecoinTransaction(tx)) {
    return tx.txDataUnion.filTxData.to ?? ''
  }

  return (
    tx.txDataUnion.ethTxData1559?.baseData.to || // EVM (1559)
    tx.txDataUnion.ethTxData?.to || // EVM
    '' // Other
  )
}

export const getToAddressesFromSolanaTransaction = (
  tx: SolanaTransactionInfo
) => {
  const { solanaTxData } = tx.txDataUnion
  const instructions = getTypedSolanaTxInstructions(solanaTxData)
  const to = solanaTxData?.toWalletAddress ?? ''

  if (to) {
    return to
  }

  const addresses = instructions.map((instruction) => {
    switch (instruction.type) {
      case 'Transfer':
      case 'TransferWithSeed':
      case 'WithdrawNonceAccount': {
        const { toPubkey } = instruction.params
        return toPubkey.toString() ?? ''
      }

      case 'Create':
      case 'CreateWithSeed': {
        const { newAccountPubkey } = instruction.params
        return newAccountPubkey.toString() ?? ''
      }

      case 'Unknown': {
        return solanaTxData?.instructions[0]?.accountMetas[0]?.pubkey.toString() ?? ''
      }

      default: return to ?? ''
    }
  })

  return [...new Set(addresses.filter(a => !!a))] // unique, non empty addresses
}

export function getTransactionToAddress (tx: BraveWallet.TransactionInfo): string {
  if (isSolanaDappTransaction(tx)) {
    return getToAddressesFromSolanaTransaction(tx)[0] ?? ''
  }

  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [recipient] = tx.txArgs // (address recipient, uint256 amount)
    return recipient
  }

  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    const [, toAddress] = tx.txArgs // (address owner, address to, uint256 tokenId)
    return toAddress
  }

  if (
    tx.txType === BraveWallet.TransactionType.ERC20Approve ||
    BraveWallet.TransactionType.ETHSwap
  ) {
    return tx.txDataUnion?.ethTxData1559?.baseData.to || ''
  }

  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData?.toWalletAddress ?? ''
  }

  if (isFilecoinTransaction(tx)) {
    return tx.txDataUnion.filTxData.to
  }

  // ETHSend & unknown
  return tx.txDataUnion.ethTxData1559?.baseData.to || ''
}

export const accountHasInsufficientFundsForGas = ({
  accountNativeBalance,
  gasFee
}: {
  accountNativeBalance: string
  gasFee: string
}): boolean => {
  return accountNativeBalance !== '' && new Amount(gasFee).gt(accountNativeBalance)
}

export const accountHasInsufficientFundsForTransaction = ({
  account,
  accountNativeBalance,
  accountTokenBalance,
  gasFee,
  nativeAsset,
  tokensList,
  tx,
  transactionNetwork,
  transferedValue
}: {
  account?: WalletAccountType
  accountNativeBalance: string
  accountTokenBalance: string
  gasFee: string
  nativeAsset: BraveWallet.BlockchainToken
  tokensList: BraveWallet.BlockchainToken[]
  tx: BraveWallet.TransactionInfo
  transactionNetwork: BraveWallet.NetworkInfo
  transferedValue: string
}): boolean => {
  const { txType, txArgs } = tx

  if (isSolanaDappTransaction(tx)) {
    return accountNativeBalance !== '' && new Amount(transferedValue)
      .plus(gasFee)
      .gt(accountNativeBalance)
  }

  // transfer(address recipient, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = txArgs // (address recipient, uint256 amount)
    return accountTokenBalance !== '' && new Amount(amount)
      .gt(accountTokenBalance)
  }

  // transferFrom(address owner, address to, uint256 tokenId)
  // safeTransferFrom(address owner, address to, uint256 tokenId)
  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    return false
  }

  // approve(address spender, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    return false // can approve for more tokens than you own
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    return accountTokenBalance !== '' && new Amount(transferedValue)
      .gt(accountTokenBalance)
  }

  // Eth Swap
  // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  if (txType === BraveWallet.TransactionType.ETHSwap) {
    const [, sellAmountArg] = txArgs // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)

    const { sellToken } = getETHSwapTranasactionBuyAndSellTokens({
      nativeAsset,
      tokensList,
      tx: tx
    })

    const sellAmountWeiBN = new Amount(sellAmountArg || transferedValue)

    const sellTokenBalance = getBalance([transactionNetwork], account, sellToken)

    return sellTokenBalance !== '' && sellAmountWeiBN.gt(sellTokenBalance)
  }

  // Others
  return accountNativeBalance !== '' && new Amount(transferedValue)
    .plus(gasFee)
    .gt(accountNativeBalance)
}

export const findTransactionToken = (
  tx: BraveWallet.TransactionInfo,
  tokensList: BraveWallet.BlockchainToken[]
) => {
  // Solana SPL
  if (isSolanaSplTransaction(tx)) {
    return findTokenByContractAddress(
      tx.txDataUnion.solanaTxData.splTokenMintAddress ?? '',
      tokensList
    )
  }

  // Solana, Filcoin & EVM
  return findTokenByContractAddress(
    getTransactionInteractionAddress(tx), // tx interacts with the contract address
    tokensList
  )
}

export const getETHSwapTranasactionBuyAndSellTokens = ({
  nativeAsset,
  tokensList,
  tx
}: {
  tx: BraveWallet.TransactionInfo
  nativeAsset: BraveWallet.BlockchainToken
  tokensList: BraveWallet.BlockchainToken[]
}): {
  buyToken: BraveWallet.BlockchainToken
  sellToken: BraveWallet.BlockchainToken
  buyAmount: Amount
  buyAmountWei: Amount
  sellAmount: Amount
  sellAmountWei: Amount
} => {
  if (tx.txType !== BraveWallet.TransactionType.ETHSwap) {
    throw new Error(
      'getETHSwapTranasactionBuyAndSellTokens must be called with an transaction with a txType of "BraveWallet.TransactionType.ETHSwap"'
    )
  }

  // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  const [fillPath, sellAmountArg, minBuyAmountArg] = tx.txArgs

  const fillContracts = fillPath
    .slice(2)
    .match(/.{1,40}/g)

  const fillTokens: BraveWallet.BlockchainToken[] = (fillContracts || [])
    .map(path => '0x' + path)
    .map(address =>
      address === NATIVE_ASSET_CONTRACT_ADDRESS_0X
        ? nativeAsset
        : findTokenByContractAddress(address, tokensList) || nativeAsset
    )

  const buyToken = fillTokens[fillTokens.length - 1]

  const sellToken = fillTokens.length === 1
    ? nativeAsset
    : fillTokens[0]

  const sellAmountWei = new Amount(
    sellAmountArg || tx.txDataUnion.ethTxData1559?.baseData.value || ''
  )
  const sellAmount = sellAmountWei.divideByDecimals(sellToken.decimals)

  const buyAmountWei = new Amount(minBuyAmountArg)
  const buyAmount = buyAmountWei
    .divideByDecimals(buyToken.decimals)

  return {
    buyToken,
    sellToken,
    sellAmount,
    buyAmount,
    buyAmountWei,
    sellAmountWei
  }
}

export const getTransactionErc721TokenId = (
  tx: BraveWallet.TransactionInfo
): string | undefined => {
  if ([
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType)) {
    const [, , tokenID] = tx.txArgs // (address owner, address to, uint256 tokenId)
    return tokenID && `#${Amount.normalize(tokenID)}`
  }
  return undefined
}

export const isSwapTransaction = (tx: BraveWallet.TransactionInfo) => {
  if (getTransactionToAddress(tx).toLowerCase() === SwapExchangeProxy) {
    return true
  }

  return [
    BraveWallet.TransactionType.ETHSwap,
    BraveWallet.TransactionType.SolanaSwap
  ].includes(tx.txType)
}

export const getTransactionNonce = (tx: BraveWallet.TransactionInfo): string => {
  return tx.txDataUnion?.ethTxData1559?.baseData.nonce || ''
}

export const getTransactionIntent = (
  tx: BraveWallet.TransactionInfo,
  transactionNetwork: BraveWallet.NetworkInfo,
  tokensList: BraveWallet.BlockchainToken[]
): string => {
  const { txType } = tx
  const token = findTransactionToken(tx, tokensList)

  const {
    normalized: transferedValue
  } = getTransactionTransferedValue({
    tx,
    txNetwork: transactionNetwork,
    token
  })

  if (isSolanaDappTransaction(tx)) {
    return txType === BraveWallet.TransactionType.SolanaSwap
      ? getLocale('braveWalletSwap')
      : getLocale('braveWalletTransactionIntentDappInteraction')
  }

  // transfer(address recipient, uint256 amount) → bool
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    return getLocale('braveWalletTransactionIntentSend').replace(
      '$1',
      transferedValue.formatAsAsset(6, token?.symbol)
    )
  }

  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    const erc721TokenId = getTransactionErc721TokenId(tx)
    return getLocale('braveWalletTransactionIntentSend').replace(
      '$1',
      `${token?.symbol ?? ''} ${erc721TokenId}`
    )
  }

  // approve(address spender, uint256 amount) → bool
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return toProperCase(getLocale('braveWalletApprovalTransactionIntent')) + ' ' + token?.symbol ?? ''
  }

  if (isSolanaSplTransaction(tx)) {
    return getLocale('braveWalletTransactionIntentSend')
      .replace(
        '$1',
        transferedValue.formatAsAsset(6, token?.symbol)
      )
  }

  // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    const {
      buyToken,
      sellToken,
      buyAmount,
      sellAmount
    } = getETHSwapTranasactionBuyAndSellTokens({
      nativeAsset: makeNetworkAsset(transactionNetwork),
      tokensList,
      tx
    })

    return getLocale('braveWalletTransactionIntentSwap')
      .replace('$1', sellAmount.formatAsAsset(6, sellToken.symbol))
      .replace('$2', buyAmount.formatAsAsset(6, buyToken.symbol))
  }

  // default / other
  return getLocale('braveWalletTransactionIntentSend').replace(
    '$1',
    transferedValue.formatAsAsset(6, transactionNetwork.symbol)
  )
}

export const addNetworkToTransaction = <T extends BraveWallet.TransactionInfo>(
  tx: T,
  networks: BraveWallet.NetworkInfo[],
  defaultNetwork: BraveWallet.NetworkInfo
): TransactionInfoWithNetworkInfo => {
  (tx as T & TransactionInfoWithNetworkInfo).network = getNetworkFromTXDataUnion(
    tx.txDataUnion,
    networks,
    defaultNetwork
  )
  return tx as T & TransactionInfoWithNetworkInfo
}

export const addAccountAddressToTransaction = <T extends BraveWallet.TransactionInfo>(
  tx: T,
  accounts: WalletAccountType[]
): T & TransactionInfoWithAccountAddress => {
  (tx as T & TransactionInfoWithAccountAddress).accountAddress = findTransactionAccount(
    accounts,
    tx
  )?.address
  return tx as T & TransactionInfoWithAccountAddress
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

export const getTransactionFiatValues = ({
  networkSpotPrice,
  solFeeEstimates,
  spotPrices,
  tokensList,
  tx,
  txNetwork,
  token
}: {
  tx: BraveWallet.TransactionInfo
  txNetwork: BraveWallet.NetworkInfo
  solFeeEstimates: SolFeeEstimates | undefined
  networkSpotPrice: string
  spotPrices: BraveWallet.AssetPrice[]
  tokensList: BraveWallet.BlockchainToken[]
  token?: BraveWallet.BlockchainToken
}): {
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  gasFeeFiat: string
} => {
  const gasFee = getGasFee(tx, solFeeEstimates)

  // return
  const gasFeeFiat = new Amount(gasFee)
    .divideByDecimals(txNetwork.decimals)
    .times(networkSpotPrice)
    .formatAsFiat()

  const { txType } = tx

  const {
    sellAmountWei,
    sellToken
  } = txType === BraveWallet.TransactionType.ETHSwap
    ? getETHSwapTranasactionBuyAndSellTokens({
        nativeAsset: makeNetworkAsset(txNetwork),
        tokensList,
        tx
      })
    : {
      sellAmountWei: undefined,
      sellToken: undefined
    }

  const {
    wei: value,
    normalized: normalizedValue
  } = getTransactionTransferedValue({
    tx,
    txNetwork,
    sellToken,
    token
  })

  if (isSolanaDappTransaction(tx)) {
    const transferedAmountFiat = computeFiatAmount(
      spotPrices,
      {
        decimals: txNetwork.decimals,
        symbol: txNetwork.symbol,
        value: normalizedValue.value?.toString() || ''
      }
    )

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(transferedAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: transferedAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: transferedAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const price = findAssetPrice(spotPrices, token?.symbol ?? '')
    const sendAmountFiat = normalizedValue.times(price)
    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sendAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sendAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the
    // caller (fromAddress).
    const totalAmountFiat = gasFeeFiat

    return {
      gasFeeFiat,
      fiatValue: Amount.zero(), // Display NFT values in the future
      fiatTotal: new Amount(totalAmountFiat),
      formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  // approve(address spender, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    const totalAmountFiat = new Amount(gasFeeFiat)
    return {
      gasFeeFiat,
      fiatValue: Amount.zero(),
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: Amount.zero()
        .formatAsAsset(2, txNetwork.symbol)
    }
  }

  if (isSolanaSplTransaction(tx)) {
    const price = findAssetPrice(spotPrices, token?.symbol ?? '')
    const sendAmountFiat = normalizedValue.times(price)
    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sendAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sendAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
    }
  }

  // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  if (txType === BraveWallet.TransactionType.ETHSwap) {
    const sellAmountFiat = computeFiatAmount(
      spotPrices,
      {
        decimals: sellToken?.decimals ?? txNetwork.decimals,
        symbol: sellToken?.symbol || '',
        value: sellAmountWei?.format() || ''
      }
    )

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sellAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sellAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sellAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
    }
  }

  // DEFAULT
  const sendAmountFiat = computeFiatAmount(spotPrices, {
    decimals: txNetwork.decimals,
    symbol: txNetwork.symbol,
    value: value.value?.toString() || ''
  })

  const totalAmountFiat = new Amount(gasFeeFiat)
    .plus(sendAmountFiat)

  return {
    gasFeeFiat,
    fiatValue: sendAmountFiat,
    fiatTotal: totalAmountFiat,
    formattedNativeCurrencyTotal: sendAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
  }
}

export const getParsedTransactionFiatValues = ({
  networkSpotPrice,
  spotPrices,
  tx
}: {
  tx: ParsedTransaction
  networkSpotPrice: string
  spotPrices: BraveWallet.AssetPrice[]
}): {
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  gasFeeFiat: string
} => {
  const gasFee = tx.gasFee
  const txNetwork = tx.network
  const token = tx?.token

  // return
  const gasFeeFiat = new Amount(gasFee)
    .divideByDecimals(txNetwork.decimals)
    .times(networkSpotPrice)
    .formatAsFiat()

  const { txType } = tx

  const {
    sellAmountWei,
    sellToken,
    weiValue,
    value
  } = tx

  const normalizedValueAmount = new Amount(value)

  if (tx.isSolanaDappTransaction) {
    const transferedAmountFiat = computeFiatAmount(
      spotPrices,
      {
        decimals: txNetwork.decimals,
        symbol: txNetwork.symbol,
        value: weiValue || ''
      }
    )

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(transferedAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: transferedAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: transferedAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const price = findAssetPrice(spotPrices, token?.symbol ?? '')
    const sendAmountFiat = normalizedValueAmount.times(price)
    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sendAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sendAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  if (
    txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the
    // caller (fromAddress).
    const totalAmountFiat = gasFeeFiat

    return {
      gasFeeFiat,
      fiatValue: Amount.zero(), // Display NFT values in the future
      fiatTotal: new Amount(totalAmountFiat),
      formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork.symbol)
    }
  }

  // approve(address spender, uint256 amount) → bool
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    const totalAmountFiat = new Amount(gasFeeFiat)
    return {
      gasFeeFiat,
      fiatValue: Amount.zero(),
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: Amount.zero()
        .formatAsAsset(2, txNetwork.symbol)
    }
  }

  if (tx.isSPLTransaction) {
    const price = findAssetPrice(spotPrices, token?.symbol ?? '')
    const sendAmountFiat = normalizedValueAmount.times(price)
    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sendAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sendAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
    }
  }

  // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
  if (txType === BraveWallet.TransactionType.ETHSwap) {
    const sellAmountFiat = computeFiatAmount(
      spotPrices,
      {
        decimals: sellToken?.decimals ?? txNetwork.decimals,
        symbol: sellToken?.symbol || '',
        value: sellAmountWei?.format() || ''
      }
    )

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sellAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sellAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sellAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
    }
  }

  // DEFAULT
  const sendAmountFiat = computeFiatAmount(spotPrices, {
    decimals: txNetwork.decimals,
    symbol: txNetwork.symbol,
    value: weiValue || ''
  })

  const totalAmountFiat = new Amount(gasFeeFiat)
    .plus(sendAmountFiat)

  return {
    gasFeeFiat,
    fiatValue: sendAmountFiat,
    fiatTotal: totalAmountFiat,
    formattedNativeCurrencyTotal: sendAmountFiat.div(networkSpotPrice).formatAsAsset(6, txNetwork.symbol)
  }
}

export const getTransactionApprovalTargetAddress = (tx: BraveWallet.TransactionInfo): string => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [spender] = tx.txArgs // (address spender, uint256 amount)
    return spender
  }

  return ''
}

export const getIsTxApprovalUnlimited = (tx: BraveWallet.TransactionInfo): boolean => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [, amount] = tx.txArgs // (address spender, uint256 amount)
    return new Amount(amount).eq(MAX_UINT256)
  }

  return false
}

export function getTransactionTokenSymbol ({
  tx,
  txNetwork,
  sellToken,
  token
}: {
  tx: BraveWallet.TransactionInfo
  txNetwork: BraveWallet.NetworkInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
}) {
  if (isSolanaDappTransaction(tx)) {
    return txNetwork.symbol
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

  return txNetwork.symbol
}

export const getLocaleKeyForTxStatus = (status: BraveWallet.TransactionStatus) => {
  switch (status) {
    case BraveWallet.TransactionStatus.Unapproved: return 'braveWalletTransactionStatusUnapproved'
    case BraveWallet.TransactionStatus.Approved: return 'braveWalletTransactionStatusApproved'
    case BraveWallet.TransactionStatus.Rejected: return 'braveWalletTransactionStatusRejected'
    case BraveWallet.TransactionStatus.Submitted: return 'braveWalletTransactionStatusSubmitted'
    case BraveWallet.TransactionStatus.Confirmed: return 'braveWalletTransactionStatusConfirmed'
    case BraveWallet.TransactionStatus.Error: return 'braveWalletTransactionStatusError'
    case BraveWallet.TransactionStatus.Dropped: return 'braveWalletTransactionStatusDropped'
    default: return ''
  }
}
