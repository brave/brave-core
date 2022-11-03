// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// types
import {
  BraveWallet,
  P3ASendTransactionTypes,
  SupportedTestNetworks,
  WalletAccountType
} from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'
import { NATIVE_ASSET_CONTRACT_ADDRESS_0X } from '../common/constants/magics'

// utils
import { getLocale } from '../../common/locale'
import { loadTimeData } from '../../common/loadTimeData'
import { getTypedSolanaTxInstructions, SolanaParamsWithLamports, TypedSolanaInstructionWithParams } from './solana-instruction-utils'
import { findTokenByContractAddress } from './asset-utils'
import Amount from './amount'

type Order = 'ascending' | 'descending'

type FileCoinTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    filTxData: BraveWallet.FilTxData
  }
}

export type SolanaTransactionInfo = BraveWallet.TransactionInfo & {
  txDataUnion: {
    solanaTxData: BraveWallet.SolanaTxData
  }
}

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

export function isSolanaTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
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

export const getTransactionNonce = (tx: BraveWallet.TransactionInfo): string => {
  return tx.txDataUnion?.ethTxData1559?.baseData.nonce || ''
}

export function isSolanaDappTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
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

export function isFilecoinTransaction (tx: BraveWallet.TransactionInfo): tx is FileCoinTransactionInfo {
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

export function isSolanaSplTransaction (tx: BraveWallet.TransactionInfo): tx is SolanaTransactionInfo {
  return (
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  )
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

export function getTransactionBaseValue (tx: BraveWallet.TransactionInfo) {
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
  tx: BraveWallet.TransactionInfo
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

export function findTransactionAccount <T extends WalletAccountType | BraveWallet.AccountInfo> (accounts: T[], transaction: BraveWallet.TransactionInfo): T | undefined {
  return accounts.find((account) =>
    account.address.toLowerCase() === transaction.fromAddress.toLowerCase()
  )
}
