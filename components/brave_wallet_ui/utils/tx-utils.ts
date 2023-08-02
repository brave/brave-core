// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityState } from '@reduxjs/toolkit'

// types
import {
  BraveWallet,
  CoinType,
  OriginInfo,
  P3ASendTransactionTypes,
  SerializableTimeDelta,
  SerializableTransactionInfo,
  SpotPriceRegistry,
  SortingOrder,
  SupportedTestNetworks,
  TimeDelta,
  TransactionInfo
} from '../constants/types'
import { SolanaTransactionTypes } from '../common/constants/solana'
import {
  MAX_UINT256,
  NATIVE_ASSET_CONTRACT_ADDRESS_0X,
  UNKNOWN_TOKEN_COINGECKO_ID
} from '../common/constants/magics'
import { SwapExchangeProxy } from '../common/constants/registry'

// utils
import { getLocale } from '../../common/locale'
import { loadTimeData } from '../../common/loadTimeData'
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
import { computeFiatAmount, getTokenPriceAmountFromRegistry } from './pricing-utils'
import { makeNetworkAsset } from '../options/asset-options'
import { getAddressLabel } from './account-utils'
import { makeSerializableTimeDelta } from './model-serialization-utils'
import { weiToEther } from './web3-utils'

export type EIP1559TransactionInfo = TransactionInfo & {
  txDataUnion: {
    ethTxData1559: BraveWallet.TxData1559
    ethTxData: undefined
    solanaTxData: undefined
    filTxData: undefined
  }
}

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
  accountAddress: string
  hash: string
  nonce: string
  createdTime: SerializableTimeDelta
  status: BraveWallet.TransactionStatus
  sender: string
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
  originInfo?: OriginInfo | undefined

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
  coinType: CoinType

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
}

export type ParsedTransactionWithoutFiatValues = Omit<
  ParsedTransaction,
  | 'fiatTotal'
  | 'fiatValue'
  | 'gasFeeFiat'
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
> (order: SortingOrder = 'ascending'): ((a: T, b: T) => number) | undefined => {
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

export function isEthereumTransaction (tx?: TransactionInfo) {
  return tx?.txDataUnion.ethTxData !== undefined || tx?.txDataUnion.ethTxData1559 !== undefined
}

export function shouldReportTransactionP3A({
  txInfo
}: {
  txInfo: Pick<
    BraveWallet.TransactionInfo | SerializableTransactionInfo,
    'txType' | 'chainId'
  > & { coinType: CoinType }
}) {
  if (
    P3ASendTransactionTypes.includes(txInfo.txType) ||
    (txInfo.coinType === CoinType.FIL &&
      txInfo.txType === BraveWallet.TransactionType.Other)
  ) {
    const countTestNetworks = loadTimeData.getBoolean(
      BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY
    )
    return countTestNetworks || !SupportedTestNetworks.includes(txInfo.chainId)
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
  const instructions = getTypedSolanaTxInstructions(solanaTxData)
  const to = solanaTxData?.toWalletAddress ?? ''

  if (to) {
    return [to]
  }

  const addresses = instructions.map(getTypedSolanaInstructionToAddress(to))

  return [...new Set(addresses.filter(a => !!a))] // unique, non empty addresses
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

  return ''
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

export const findTransactionToken = <
  T extends Pick<BraveWallet.BlockchainToken, 'contractAddress'>
>(
  tx: TransactionInfo | undefined,
  tokensList: T[]
): T | undefined => {
  if (!tx) {
    return undefined
  }

  // Solana SPL
  if (isSolanaSplTransaction(tx)) {
    return findTokenByContractAddress(
      tx.txDataUnion.solanaTxData.splTokenMintAddress ?? '',
      tokensList
    )
  }

  // Solana, Filecoin & EVM
  return findTokenByContractAddress(
    getTransactionInteractionAddress(tx), // tx interacts with the contract address
    tokensList
  )
}

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
    tx.txType !== BraveWallet.TransactionType.ETHSwap
  ) {
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
        : findTokenByContractAddress(address, tokensList) ||
        // token not found
        // return a "faked" coin (will need to "discover" it later)
        {
          chainId: tx.chainId,
          coin: getCoinFromTxDataUnion(tx.txDataUnion),
          contractAddress: address,
          symbol: '???',
          isErc20: true,
          coingeckoId: UNKNOWN_TOKEN_COINGECKO_ID,
          name: address,
          logo: 'chrome://erc-token-images/',
          tokenId: '',
          isErc1155: false,
          isErc721: false,
          isNft: false,
          isSpam: false,
          visible: true
        } as BraveWallet.BlockchainToken
    ).filter((t): t is BraveWallet.BlockchainToken => Boolean(t))

  const sellToken = fillTokens.length === 1
    ? nativeAsset
    : fillTokens[0]


  const sellAmountRaw = sellToken?.contractAddress === ''
    ? (tx.txDataUnion.ethTxData1559?.baseData.value ||
       tx.txDataUnion.ethTxData?.value ||
       sellAmountArg ||
       '')
    : sellAmountArg || ''

  const sellAmountWei = new Amount(sellAmountRaw)

  const sellAmount = sellToken
    ? sellAmountWei.divideByDecimals(sellToken.decimals)
    : Amount.empty()

  const buyToken = fillTokens[fillTokens.length - 1]
  const buyAmountWei = new Amount(minBuyAmountArg)
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

export function getLamportsMovedFromInstructions (
  instructions: TypedSolanaInstructionWithParams[],
  fromAddress: string
) {
  return instructions.reduce((acc, instruction) => {
    const { lamports } = getSolInstructionParamsObj(instruction.params)

    const {
      fromAccount,
      nonceAccount,
      toAccount
    } = getSolInstructionAccountParamsObj(
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

      default: return acc.plus(lamports)
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

    const transferredValue = new Amount(getTransactionBaseValue(tx))
      .plus(lamportsMovedFromInstructions)

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

export const findTransactionAccount = <T extends { address: string }>(
  accounts: T[],
  transaction: { fromAddress: string }
): T | undefined => {
  return accounts.find(
    (account) =>
      account.address.toLowerCase() === transaction.fromAddress.toLowerCase()
  )
}

export const findTransactionAccountFromRegistry = <
  T extends { address: string }
>(
  registry: EntityState<T>,
  transaction: { fromAddress: string }
): T | undefined => {
  const id = registry.ids.find(
    (accountId) =>
      accountId.toString().toLowerCase() ===
      transaction.fromAddress.toLowerCase()
  )

  return id ? registry.entities[id] : undefined
}

export function getTransactionGasLimit (transaction: TransactionInfo) {
  return isFilecoinTransaction(transaction)
    ? transaction.txDataUnion.filTxData.gasLimit
    : transaction.txDataUnion.ethTxData1559?.baseData.gasLimit || ''
}

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
): string => {
  const { maxFeePerGas, gasPrice } = getTransactionGas(transaction)
  const gasLimit = getTransactionGasLimit(transaction)

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
  tx: TransactionInfo
) => {
  const gasLimit = getTransactionGasLimit(tx)
  const { gasPrice, maxFeePerGas, maxPriorityFeePerGas } = getTransactionGas(tx)

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
    case CoinType.SOL: return 9
    case CoinType.ETH: return 18
    case CoinType.FIL: return 18
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
function isKnownTokenContractAddress(
  address: string,
  tokenList: BraveWallet.BlockchainToken[]
) {
  return tokenList?.some(
    (token) => token.contractAddress.toLowerCase() === address.toLowerCase()
  )
}

/**
 * Checks if a given transaction is sending funds to a known contract address from our token registry.
 *
 * @param tokenList - A list of Erc & SPL tokens to check against
 * @param tx - The transaction to check
 * @returns `true` if the to address is a known erc & SPL token contract address, `false` otherwise
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
    // The owner of the ERC721 must not be confused with the caller (fromAddress).
    const [, toAddress] = tx.txArgs // address owner, address to, uint256 tokenId]
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
    tx.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
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
      tx.fromAddress
    )

    const transferredValue = new Amount(getTransactionBaseValue(tx))
      .plus(lamportsMovedFromInstructions)

    return accountNativeBalance !== '' && transferredValue
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
    return sellTokenBalance !== '' && sellAmountWei.gt(sellTokenBalance)
  }
  // ETHSend
  // SolanaSystemTransfer
  // Other
  return accountNativeBalance !== '' && new Amount(getTransactionBaseValue(tx))
    .plus(gasFee)
    .gt(accountNativeBalance)
}

export function getTransactionTransferredToken ({
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

export function getTransactionTokenSymbol ({
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

export const getTransactionFormattedNativeCurrencyTotal = ({
  gasFee,
  normalizedTransferredValue,
  sellAmountWei,
  sellToken,
  token,
  transferredValueWei,
  tx,
  txNetwork
}: {
  gasFee: string
  normalizedTransferredValue: string
  sellAmountWei?: string
  sellToken?: BraveWallet.BlockchainToken
  token?: BraveWallet.BlockchainToken
  transferredValueWei?: string
  tx: TransactionInfo
  txNetwork?: BraveWallet.NetworkInfo
}): string => {
  // Solana Dapps
  if (isSolanaDappTransaction(tx)) {
    const transferredAmount = txNetwork
      ? weiToEther(transferredValueWei || '', txNetwork.decimals)
      : Amount.empty()

    return transferredAmount.formatAsAsset(6, txNetwork?.symbol)
  }

  // ERC20 Transfer
  if (tx.txType === BraveWallet.TransactionType.ERC20Transfer) {
    const [, amount] = tx.txArgs // (address recipient, uint256 amount) → bool

    const sendAmount = weiToEther(amount, token?.decimals ?? 18)

    return sendAmount
      .formatAsAsset(6, txNetwork?.symbol)
  }

  // ERC721 TransferFrom
  if (
    tx.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
    tx.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  ) {
    // The owner of the ERC721 must not be confused with the
    // caller (fromAddress).
    const normalizedGasFee = txNetwork
      ? new Amount(gasFee).divideByDecimals(txNetwork.decimals)
      : new Amount('')
    return normalizedGasFee.formatAsAsset(
      6,
      txNetwork?.symbol
    )
  }

  // ERC20 Approve
  if (tx.txType === BraveWallet.TransactionType.ERC20Approve) {
    return Amount.zero().formatAsAsset(
      2,
      txNetwork?.symbol
    )
  }

  // SPL
  if (isSolanaSplTransaction(tx)) {
    return new Amount(normalizedTransferredValue)
      .formatAsAsset(6, txNetwork?.symbol)
  }

  // ETH SWAP
  if (tx.txType === BraveWallet.TransactionType.ETHSwap) {
    const sellAmount =
      sellToken && sellAmountWei
        ? weiToEther(sellAmountWei, sellToken.decimals)
        : Amount.empty()

    return sellAmount
      .formatAsAsset(6, txNetwork?.symbol)
  }

  // DEFAULT
  const sendAmount = txNetwork
    ? weiToEther(getTransactionBaseValue(tx) || '', txNetwork.decimals)
    : Amount.empty()

  return sendAmount
    .formatAsAsset(6, txNetwork?.symbol)
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
  return new Amount(normalizedTransferredValue)
    .formatAsAsset(6, sendToken?.symbol)
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
  transactionNetwork,
  tokensList
}: {
  accounts: BraveWallet.AccountInfo[]
  tx: TransactionInfo
  transactionNetwork?: BraveWallet.NetworkInfo
  tokensList: BraveWallet.BlockchainToken[]
}): ParsedTransactionWithoutFiatValues => {
  const to = getTransactionToAddress(tx)
  const token = findTransactionToken(tx, tokensList)
  const nativeAsset = makeNetworkAsset(transactionNetwork)
  const account = findTransactionAccount(accounts, tx)

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
    accountAddress: account?.address || '',
    approvalTarget,
    approvalTargetLabel,
    buyToken,
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
    isSolanaDappTransaction: isSolanaTransaction(tx),
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
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol,
    token,
    txType: tx.txType,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact,
    weiTransferredValue,
    formattedSendCurrencyTotal
  }
}

export const parseTransactionWithPrices = ({
  accounts,
  tx,
  transactionNetwork,
  spotPriceRegistry,
  gasFee,
  tokensList
}: {
  accounts: BraveWallet.AccountInfo[]
  tx: TransactionInfo
  transactionNetwork?: BraveWallet.NetworkInfo
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
    transactionNetwork,
    tx,
    tokensList
  })

  return {
    token,
    sellToken,
    weiTransferredValue,
    value: normalizedTransferredValue,
    ...txBase,
    ...getTransactionFiatValues({
      gasFee,
      sellAmountWei: sellAmountWei?.value?.toString(),
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
