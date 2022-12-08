// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// types
import {
  BraveWallet,
  P3ASendTransactionTypes,
  SolFeeEstimates,
  SupportedTestNetworks,
  WalletAccountType,
  SerializableTransactionInfo
} from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'
import { MAX_UINT256, NATIVE_ASSET_CONTRACT_ADDRESS_0X } from '../common/constants/magics'
import { SwapExchangeProxy } from '../common/constants/registry'

// utils
import { getLocale } from '../../common/locale'
import { loadTimeData } from '../../common/loadTimeData'
import { getTypedSolanaTxInstructions, SolanaParamsWithLamports, TypedSolanaInstructionWithParams } from './solana-instruction-utils'
import { findTokenByContractAddress } from './asset-utils'
import Amount from './amount'
import { getCoinFromTxDataUnion } from './network-utils'
import { getBalance } from './balance-utils'
import { toProperCase } from './string-utils'
import { computeFiatAmount, findAssetPrice } from './pricing-utils'

type Order = 'ascending' | 'descending'

export type TransactionInfo = BraveWallet.TransactionInfo | SerializableTransactionInfo

type EIP1559TransactionInfo = TransactionInfo & {
  txDataUnion: {
    ethTxData1559: BraveWallet.TxData1559
  }
}

type FileCoinTransactionInfo = TransactionInfo & {
  txDataUnion: {
    filTxData: BraveWallet.FilTxData
  }
}

export type SolanaTransactionInfo = TransactionInfo & {
  txDataUnion: {
    solanaTxData: BraveWallet.SolanaTxData
  }
}

export const sortTransactionByDate = <T extends TransactionInfo>(
  transactions: T[],
  order: Order = 'ascending'
): T[] => {
  return [...transactions].sort(function (x: T, y: T) {
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
    case BraveWallet.TransactionStatus.Signed:
      return getLocale('braveWalletTransactionStatusSigned')
    default:
      return ''
  }
}

export function isSolanaTransaction (tx: TransactionInfo): tx is SolanaTransactionInfo {
  const { txType, txDataUnion: { solanaTxData } } = tx
  return SolanaTransactionTypes.includes(txType) ||
    (txType === BraveWallet.TransactionType.Other && solanaTxData !== undefined)
}

export function shouldReportTransactionP3A (txInfo: BraveWallet.TransactionInfo, network: BraveWallet.NetworkInfo, coin: BraveWallet.CoinType) {
  if (P3ASendTransactionTypes.includes(txInfo.txType) ||
    (coin === BraveWallet.CoinType.FIL && txInfo.txType === BraveWallet.TransactionType.Other)) {
    const countTestNetworks = loadTimeData.getBoolean(BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY)
    return countTestNetworks || !SupportedTestNetworks.includes(network.chainId)
  }
  return false
}

export const getTransactionNonce = (tx: TransactionInfo): string => {
  return tx.txDataUnion?.ethTxData1559?.baseData.nonce || ''
}

export function isSolanaDappTransaction (tx: TransactionInfo): tx is SolanaTransactionInfo {
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

export function isFilecoinTransaction (tx: TransactionInfo): tx is FileCoinTransactionInfo {
  return tx.txDataUnion.filTxData !== undefined
}

export const getToAddressesFromSolanaTransaction = (
  tx: SolanaTransactionInfo
) => {
  const { solanaTxData } = tx.txDataUnion
  const instructions = getTypedSolanaTxInstructions(solanaTxData)
  const to = solanaTxData?.toWalletAddress ?? ''

  if (to) {
    return [to]
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

export function getTransactionToAddress (tx: TransactionInfo): string {
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

export function getTransactionInteractionAddress (tx: TransactionInfo): string {
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

export function isSolanaSplTransaction (tx: TransactionInfo): tx is SolanaTransactionInfo {
  return (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  )
}

export const findTransactionToken = (
  tx: TransactionInfo,
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
  tx: TransactionInfo
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
  if (tx.txType !== BraveWallet.TransactionType.ETHSwap) {
    return {
      buyToken: undefined,
      sellToken: undefined,
      buyAmount: new Amount(''),
      sellAmount: new Amount(''),
      sellAmountWei: new Amount(''),
      buyAmountWei: new Amount('')
    }
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
    ).filter((t): t is BraveWallet.BlockchainToken => Boolean(t))

  const sellToken = fillTokens.length === 1
    ? nativeAsset
    : fillTokens[0]

  const sellAmountWei = new Amount(sellToken
    ? sellAmountArg || tx.txDataUnion.ethTxData1559?.baseData.value || ''
    : ''
  )

  const sellAmount = sellToken
    ? sellAmountWei.divideByDecimals(sellToken.decimals)
    : Amount.empty()

  const buyToken = fillTokens[fillTokens.length - 1]
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

export function getTransactionBaseValue (tx: TransactionInfo) {
  if (isSolanaSplTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.amount.toString() ?? ''
  }

  if (isSolanaTransaction(tx)) {
    return tx.txDataUnion.solanaTxData.lamports.toString() ?? ''
  }

  if (isFilecoinTransaction(tx)) {
    return tx.txDataUnion.filTxData.value || ''
  }

  return tx.txDataUnion.ethTxData1559?.baseData.value || ''
}

export function getTransactionTransferredValue ({
  tx,
  txNetwork,
  token,
  sellToken
}: {
  tx: TransactionInfo
  token?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
  txNetwork?: BraveWallet.NetworkInfo
}): {
  wei: Amount
  normalized: Amount
} {
  // Can't compute value with network decimals if no network was provided
  if (!txNetwork) {
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
    // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
    const [, sellAmountArg] = tx.txArgs
    const wei = new Amount(sellAmountArg || getTransactionBaseValue(tx))
    return {
      wei,
      normalized: sellToken
        ? wei.divideByDecimals(sellToken.decimals ?? txNetwork.decimals)
        : Amount.empty()
    }
  }

  // Solana Dapp Transactions
  if (isSolanaDappTransaction(tx)) {
    const lamportsMovedFromInstructions = getLamportsMovedFromInstructions(
      getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData) || [],
      tx.fromAddress
    )

    const transferedValue = new Amount(getTransactionBaseValue(tx))
      .plus(lamportsMovedFromInstructions)

    return {
      wei: transferedValue,
      normalized: transferedValue.divideByDecimals(txNetwork.decimals)
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

export function getFormattedTransactionTransferredValue (
  args: Parameters<typeof getTransactionTransferredValue>[0]
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

export function findTransactionAccount <T extends WalletAccountType | BraveWallet.AccountInfo> (accounts: T[], transaction: TransactionInfo): T | undefined {
  return accounts.find((account) =>
    account.address.toLowerCase() === transaction.fromAddress.toLowerCase()
  )
}

export function getTransactionGasLimit (transaction: TransactionInfo) {
  return isFilecoinTransaction(transaction)
    ? transaction.txDataUnion.filTxData.gasLimit
    : transaction.txDataUnion.ethTxData1559?.baseData.gasLimit || ''
}

export function getTransactionGas (transaction: TransactionInfo): { gasPrice: string, maxFeePerGas: any, maxPriorityFeePerGas: any } {
  return {
    gasPrice: transaction.txDataUnion.ethTxData1559?.baseData.gasPrice || '',
    maxFeePerGas: transaction.txDataUnion.ethTxData1559?.maxFeePerGas || '',
    maxPriorityFeePerGas: transaction.txDataUnion.ethTxData1559?.maxPriorityFeePerGas || ''
  }
}

export const isEIP1559Transaction = (transaction: TransactionInfo): transaction is EIP1559TransactionInfo => {
  const { maxFeePerGas, maxPriorityFeePerGas } = getTransactionGas(transaction)
  return maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
}

/**
 * @param transaction the transaction to check
 * @param solFeeEstimates [FIXME] - Extract actual fees used in the Solana transaction, instead of populating current estimates.
 * @returns string value of the gas fee
 */
export const getTransactionGasFee = (
  transaction: TransactionInfo,
  solFeeEstimates: SolFeeEstimates | undefined
): string => {
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
 * Checks if a given transaction has an empty or zero-value gas limit
 *
 * @param tx - The transaction to check for a missing gas limit
 * @returns `true`, if the gas limit is missing, `false` otherwise
*/
export const isTransactionGasLimitMissing = (tx: TransactionInfo): boolean => {
  if (isSolanaTransaction(tx)) {
    return false
  }

  const gasLimit = getTransactionGasLimit(tx)
  return (gasLimit === '' || Amount.normalize(gasLimit) === '0')
}

export const parseTransactionFeesWithoutPrices = (
  tx: TransactionInfo,
  solFeeEstimates?: SolFeeEstimates
) => {
  const gasLimit = getTransactionGasLimit(tx)
  const {
    gasPrice,
    maxFeePerGas,
    maxPriorityFeePerGas
  } = getTransactionGas(tx)

  const gasFee = getTransactionGasFee(tx, solFeeEstimates)

  return {
    gasLimit: Amount.normalize(gasLimit),
    gasPrice: Amount.normalize(gasPrice),
    maxFeePerGas: Amount.normalize(maxFeePerGas),
    maxPriorityFeePerGas: Amount.normalize(maxPriorityFeePerGas),
    gasFee,
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

export const getTransactionApprovalTargetAddress = (tx: TransactionInfo): string => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [spender] = tx.txArgs // (address spender, uint256 amount)
    return spender
  }

  return ''
}

export function getTransactionDecimals ({
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
    case BraveWallet.CoinType.SOL: return 9
    case BraveWallet.CoinType.ETH: return 18
    case BraveWallet.CoinType.FIL: return 18
    default: return 18
  }
}

export const getTransactionErc721TokenId = (
  tx: TransactionInfo
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
function isKnownTokenContractAddress (
  address: string,
  fullTokenList: BraveWallet.BlockchainToken[]
) {
  return fullTokenList?.some(token =>
    token.contractAddress.toLowerCase() === address.toLowerCase()
  )
}

/**
 * Checks if a given transaction is sending funds to a known contract address from our token registry.
 *
 * @param fullTokenList - A list of Erc & SPL tokens to check against
 * @param tx - The transaction to check
 * @returns `true` if the to address is a known erc & SPL token contract address, `false` otherwise
*/
export const isSendingToKnownTokenContractAddress = (
  tx: TransactionInfo,
  fullTokenList: BraveWallet.BlockchainToken[]
): boolean => {
  // ERC20Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [recipient] = tx.txArgs // [address recipient, uint256 amount]
    const contractAddressError = isKnownTokenContractAddress(recipient, fullTokenList)
    return contractAddressError
  }

  // ERC721TransferFrom
  // ERC721SafeTransferFrom
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the caller (fromAddress).
    const [, toAddress] = tx.txArgs // address owner, address to, uint256 tokenId]
    const contractAddressError = isKnownTokenContractAddress(toAddress, fullTokenList)
    return contractAddressError
  }

  // ERC20Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return false
  }

  // Solana SPL Token Transfer
  if (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  ) {
    const contractAddressError = isKnownTokenContractAddress(
      getTransactionInteractionAddress(tx) ?? '',
      fullTokenList
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
    // The owner of the ERC721 must not be confused with the caller (fromAddress).
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

export function getGasFeeFiatValue ({
  gasFee,
  networkSpotPrice,
  txNetwork
}: {
  gasFee: string
  networkSpotPrice?: string
  txNetwork?: BraveWallet.NetworkInfo
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
  account,
  accountNativeBalance,
  accountTokenBalance,
  gasFee,
  nativeAsset,
  tokensList,
  tx
}: {
  account?: WalletAccountType
  accountNativeBalance: string
  accountTokenBalance: string
  gasFee: string
  nativeAsset?: BraveWallet.BlockchainToken
  tokensList: BraveWallet.BlockchainToken[]
  tx: TransactionInfo
}): boolean => {
  const { txType, txArgs } = tx

  if (isSolanaDappTransaction(tx)) {
    const lamportsMovedFromInstructions = getLamportsMovedFromInstructions(
      getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData) || [],
      tx.fromAddress
    )

    const transferedValue = new Amount(getTransactionBaseValue(tx))
      .plus(lamportsMovedFromInstructions)

    return accountNativeBalance !== '' && transferedValue
      .plus(gasFee)
      .gt(accountNativeBalance)
  }

  // ERC20
  if (txType === BraveWallet.TransactionType.ERC20Approve) {
    return false // can approve for more tokens than you own
  }

  if (txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = txArgs // (address recipient, uint256 amount)
    return accountTokenBalance !== '' && new Amount(amount)
      .gt(accountTokenBalance)
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
    return accountTokenBalance !== '' && new Amount(getTransactionBaseValue(tx))
      .gt(accountTokenBalance)
  }

  // Eth Swap
  if (txType === BraveWallet.TransactionType.ETHSwap) {
    const {
      sellToken,
      sellAmountWei
    } = getETHSwapTranasactionBuyAndSellTokens({
      nativeAsset,
      tokensList,
      tx
    })
    const sellTokenBalance = getBalance(account, sellToken)

    return sellTokenBalance !== '' && sellAmountWei.gt(sellTokenBalance)
  }
  // ETHSend
  // SolanaSystemTransfer
  // Other
  return accountNativeBalance !== '' && new Amount(getTransactionBaseValue(tx))
    .plus(gasFee)
    .gt(accountNativeBalance)
}

export function getTransactionTokenSymbol ({
  tx,
  txNetwork,
  sellToken,
  token
}: {
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
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
    return toProperCase(getLocale('braveWalletApprovalTransactionIntent')) + ' ' + token?.symbol ?? ''
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
    return getLocale('braveWalletTransactionIntentSend')
      .replace(
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
  return accountNativeBalance !== '' && new Amount(gasFee).gt(accountNativeBalance)
}

export const getIsTxApprovalUnlimited = (tx: TransactionInfo): boolean => {
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    const [, amount] = tx.txArgs // (address spender, uint256 amount)
    return new Amount(amount).eq(MAX_UINT256)
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

export const getTransactionFiatValues = ({
  gasFee,
  networkSpotPrice,
  normalizedTransferredValue,
  sellAmountWei,
  sellToken,
  spotPrices,
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
  spotPrices: BraveWallet.AssetPrice[]
  token?: BraveWallet.BlockchainToken
  transferredValueWei?: string
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
}): {
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  gasFeeFiat: string
} => {
  const gasFeeFiat = getGasFeeFiatValue({
    gasFee,
    networkSpotPrice,
    txNetwork
  })

  // Solana Dapps
  if (isSolanaDappTransaction(tx)) {
    const transferedAmountFiat = txNetwork
      ? computeFiatAmount(
        spotPrices,
        {
          decimals: txNetwork.decimals,
          symbol: txNetwork.symbol,
          value: transferredValueWei || ''
        }
      )
    : Amount.empty()

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(transferedAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: transferedAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: transferedAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork?.symbol)
    }
  }

  // ERC20 Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = tx.txArgs // (address recipient, uint256 amount) → bool

    const price = findAssetPrice(spotPrices, token?.symbol ?? '')

    const sendAmountFiat = new Amount(amount)
      .divideByDecimals(token?.decimals ?? 18)
      .times(price)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: new Amount(gasFeeFiat).plus(sendAmountFiat),
      formattedNativeCurrencyTotal: sendAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork?.symbol)
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
      fiatValue: Amount.zero(), // Display NFT values in the future
      fiatTotal: new Amount(totalAmountFiat),
      formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork?.symbol)
    }
  }

  // ERC20 Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return {
      gasFeeFiat,
      fiatValue: Amount.zero(),
      fiatTotal: new Amount(gasFeeFiat),
      formattedNativeCurrencyTotal: Amount.zero()
        .formatAsAsset(2, txNetwork?.symbol)
    }
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    const price = findAssetPrice(spotPrices, token?.symbol ?? '')
    const sendAmountFiat = new Amount(normalizedTransferredValue).times(price)

    return {
      gasFeeFiat,
      fiatValue: sendAmountFiat,
      fiatTotal: new Amount(gasFeeFiat).plus(sendAmountFiat),
      formattedNativeCurrencyTotal: sendAmountFiat
        .div(networkSpotPrice)
        .formatAsAsset(6, txNetwork?.symbol)
    }
  }

  // ETH SWAP
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    const sellAmountFiat = sellToken && sellAmountWei
      ? computeFiatAmount(
          spotPrices,
          {
            decimals: sellToken.decimals,
            symbol: sellToken.symbol,
            value: sellAmountWei
          }
        )
      : Amount.empty()

    const totalAmountFiat = new Amount(gasFeeFiat)
      .plus(sellAmountFiat)

    return {
      gasFeeFiat,
      fiatValue: sellAmountFiat,
      fiatTotal: totalAmountFiat,
      formattedNativeCurrencyTotal: sellAmountFiat.div(networkSpotPrice)
        .formatAsAsset(6, txNetwork?.symbol)
    }
  }

  // DEFAULT
  const sendAmountFiat = txNetwork
    ? computeFiatAmount(spotPrices, {
      decimals: txNetwork.decimals,
      symbol: txNetwork.symbol,
      value: getTransactionBaseValue(tx) || ''
    })
    : Amount.empty()

  return {
    gasFeeFiat,
    fiatValue: sendAmountFiat,
    fiatTotal: new Amount(gasFeeFiat)
      .plus(sendAmountFiat),
    formattedNativeCurrencyTotal: sendAmountFiat.div(networkSpotPrice)
      .formatAsAsset(6, txNetwork?.symbol)
  }
}
