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
import {
  MAX_UINT256,
  NATIVE_ASSET_CONTRACT_ADDRESS_0X
} from '../constants/magics'
import { SwapExchangeProxy } from '../constants/registry'

// Utils
import { getLocale } from '../../../common/locale'
import Amount from '../../utils/amount'
import {
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from '../../utils/solana-instruction-utils'
import {
  accountHasInsufficientFundsForTransaction,
  findTransactionAccount,
  findTransactionToken,
  getETHSwapTranasactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getGasFeeFiatValue,
  getTransactionApprovalTargetAddress,
  getTransactionBaseValue,
  getTransactionDecimals,
  getTransactionErc721TokenId,
  getTransactionNonce,
  getTransactionToAddress,
  isFilecoinTransaction,
  isSendingToKnownTokenContractAddress,
  isSolanaDappTransaction,
  isSolanaSplTransaction,
  isSolanaTransaction,
  parseTransactionFeesWithoutPrices,
  transactionHasSameAddressError
} from '../../utils/tx-utils'
import { getBalance } from '../../utils/balance-utils'
import { getAddressLabel } from '../../utils/account-utils'
import { toProperCase } from '../../utils/string-utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { findTokenByContractAddress } from '../../utils/asset-utils'
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

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransaction => {
    const { txArgs, txType } = transactionInfo

    const {
      gasFee,
      gasFeeCap,
      gasLimit,
      gasPremium,
      gasPrice,
      isMissingGasLimit,
      maxFeePerGas,
      isEIP1559Transaction,
      maxPriorityFeePerGas
    } = parseTransactionFeesWithoutPrices(transactionInfo, solFeeEstimates)
    const gasFeeFiat = getGasFeeFiatValue({
      gasFee,
      networkSpotPrice,
      txNetwork: transactionNetwork
    })

    const isFilTransaction = isFilecoinTransaction(transactionInfo)
    const isSolanaTxn = isSolanaTransaction(transactionInfo)
    const isSPLTransaction = isSolanaSplTransaction(transactionInfo)
    const baseValue = getTransactionBaseValue(transactionInfo)
    const to = getTransactionToAddress(transactionInfo)
    const nonce = getTransactionNonce(transactionInfo)
    const account = findTransactionAccount(accounts, transactionInfo)
    const token = findTransactionToken(transactionInfo, combinedTokensList)
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
      tx: transactionInfo
    })

    const {
      normalizedTransferredValue,
      normalizedTransferredValueExact,
      weiTransferredValue
    } = getFormattedTransactionTransferredValue({
      tx: transactionInfo,
      txNetwork: transactionNetwork,
      token,
      sellToken
    })

    const erc721Token = [
      BraveWallet.TransactionType.ERC721TransferFrom,
      BraveWallet.TransactionType.ERC721SafeTransferFrom
    ].includes(transactionInfo.txType) ? token : undefined

    const erc721TokenId = getTransactionErc721TokenId(transactionInfo)
    const approvalTarget = getTransactionApprovalTargetAddress(transactionInfo)

    const txBase: Pick<
      ParsedTransaction,
      | 'approvalTarget'
      | 'approvalTargetLabel'
      | 'buyToken'
      | 'coinType'
      | 'contractAddressError'
      | 'createdTime'
      | 'decimals'
      | 'erc721BlockchainToken'
      | 'erc721TokenId'
      | 'gasFee'
      | 'gasFeeCap'
      | 'gasFeeFiat'
      | 'gasLimit'
      | 'missingGasLimitError'
      | 'gasPremium'
      | 'gasPrice'
      | 'hash'
      | 'id'
      | 'isEIP1559Transaction'
      | 'instructions'
      | 'insufficientFundsForGasError'
      | 'isFilecoinTransaction'
      | 'isSolanaDappTransaction'
      | 'isSolanaSPLTransaction'
      | 'isSolanaTransaction'
      | 'maxFeePerGas'
      | 'maxPriorityFeePerGas'
      | 'minBuyAmount'
      | 'minBuyAmountWei'
      | 'nonce'
      | 'recipient'
      | 'recipientLabel'
      | 'sameAddressError'
      | 'sellAmount'
      | 'sellAmountWei'
      | 'sellToken'
      | 'sender'
      | 'senderLabel'
      | 'token'
      | 'value'
      | 'valueExact'
    > = {
      approvalTarget,
      approvalTargetLabel: getAddressLabel(approvalTarget, accounts),
      buyToken,
      coinType: getCoinFromTxDataUnion(transactionInfo.txDataUnion),
      createdTime: transactionInfo.createdTime,
      contractAddressError: isSendingToKnownTokenContractAddress(transactionInfo, combinedTokensList)
        ? getLocale('braveWalletContractAddressError')
        : undefined,
      decimals: getTransactionDecimals({
        tx: transactionInfo,
        network: transactionNetwork,
        sellToken,
        erc721Token,
        token
      }),
      erc721BlockchainToken: erc721Token,
      erc721TokenId,
      gasFeeFiat,
      gasFee,
      gasFeeCap,
      gasLimit,
      gasPremium,
      gasPrice,
      maxFeePerGas,
      maxPriorityFeePerGas,
      missingGasLimitError: isMissingGasLimit
        ? getLocale('braveWalletMissingGasLimitError')
        : undefined,
      hash: transactionInfo.txHash,
      id: transactionInfo.id,
      isEIP1559Transaction,
      instructions: getTypedSolanaTxInstructions(transactionInfo.txDataUnion.solanaTxData),
      isFilecoinTransaction: isFilTransaction,
      isSolanaDappTransaction: isSolanaDappTransaction(transactionInfo),
      isSolanaSPLTransaction: isSPLTransaction,
      isSolanaTransaction: isSolanaTxn,
      minBuyAmount: buyAmount,
      minBuyAmountWei: buyAmountWei,
      nonce,
      recipient: to,
      recipientLabel: getAddressLabel(to, accounts),
      sameAddressError: transactionHasSameAddressError(transactionInfo) ? getLocale('braveWalletSameAddressError') : undefined,
      sellAmount,
      sellAmountWei,
      sellToken,
      sender: transactionInfo.fromAddress,
      senderLabel: getAddressLabel(transactionInfo.fromAddress, accounts),
      token,
      value: normalizedTransferredValue,
      valueExact: normalizedTransferredValueExact
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
          status: transactionInfo.txStatus,
          recipient: to,
          recipientLabel: getAddressLabel(to, accounts),
          fiatValue: transferedAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: transferedAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          symbol: selectedNetwork?.symbol ?? '',
          insufficientFundsError: accountNativeBalance !== ''
            ? new Amount(normalizedTransferredValue).plus(gasFee).gt(accountNativeBalance)
            : undefined,
          insufficientFundsForGasError: accountNativeBalance !== ''
            ? new Amount(gasFee).gt(accountNativeBalance)
            : undefined,
          isSwap: txType === BraveWallet.TransactionType.SolanaSwap,
          intent: txType === BraveWallet.TransactionType.SolanaSwap
            ? getLocale('braveWalletSwap')
            : getLocale('braveWalletTransactionIntentDappInteraction')
        }

        return parsedTx
      }

      // transfer(address recipient, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Transfer: {
        const [, amount] = txArgs
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(amount)
          .divideByDecimals(token?.decimals ?? 18)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined
        const insufficientTokenFunds = accountTokenBalance !== ''
          ? new Amount(amount).gt(accountTokenBalance)
          : undefined

        return {
          ...txBase,
          status: transactionInfo.txStatus,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          symbol: token?.symbol ?? '',
          insufficientFundsError: insufficientTokenFunds,
          insufficientFundsForGasError: insufficientNativeFunds,
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', new Amount(normalizedTransferredValue).formatAsAsset(6, token?.symbol))
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721SafeTransferFrom: {
        const totalAmountFiat = gasFeeFiat

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        return {
          ...txBase,
          status: transactionInfo.txStatus, // The caller, which may not be the owner
          fiatValue: Amount.zero(), // Display NFT values in the future
          fiatTotal: new Amount(totalAmountFiat),
          formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          symbol: token?.symbol ?? '',
          insufficientFundsForGasError: insufficientNativeFunds,
          insufficientFundsError: false,
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', `${token?.symbol ?? ''} ${erc721TokenId}`)
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Approve: {
        const totalAmountFiat = new Amount(gasFeeFiat)
        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        return {
          ...txBase,
          status: transactionInfo.txStatus,
          fiatValue: Amount.zero(),
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: Amount.zero()
            .formatAsAsset(2, selectedNetwork?.symbol),
          symbol: token?.symbol ?? '',
          isApprovalUnlimited: new Amount(weiTransferredValue).eq(MAX_UINT256),
          insufficientFundsForGasError: insufficientNativeFunds,
          insufficientFundsError: false,
          intent: toProperCase(getLocale('braveWalletApprovalTransactionIntent')) + ' ' + token?.symbol ?? ''
        } as ParsedTransaction
      }

      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer:
      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation: {
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(normalizedTransferredValue)
          .times(price)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        return {
          ...txBase,
          status: transactionInfo.txStatus,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: normalizedTransferredValue,
          valueExact: normalizedTransferredValueExact,
          symbol: token?.symbol ?? '',
          insufficientFundsError: accountHasInsufficientFundsForTransaction({
            accountNativeBalance,
            accountTokenBalance,
            gasFee,
            tokensList: combinedTokensList,
            tx: transactionInfo,
            account,
            nativeAsset
          }),
          insufficientFundsForGasError: insufficientNativeFunds,
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', new Amount(normalizedTransferredValue).formatAsAsset(6, token?.symbol))
        } as ParsedTransaction
      }

      // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
      case txType === BraveWallet.TransactionType.ETHSwap: {
        const [fillPath, , minBuyAmountArg] = txArgs

        const fillContracts = fillPath
          .slice(2)
          .match(/.{1,40}/g)
        const fillTokens = (fillContracts || [])
          .map(path => '0x' + path)
          .map(address =>
            address === NATIVE_ASSET_CONTRACT_ADDRESS_0X
              ? nativeAsset
              : findTokenByContractAddress(address, combinedTokensList) || nativeAsset)
          .filter(Boolean) as BraveWallet.BlockchainToken[]

        const sellToken = fillTokens.length === 1
          ? nativeAsset
          : fillTokens[0]

        const sellAmountFiat = sellToken
          ? computeFiatAmount(
              weiTransferredValue,
              sellToken.symbol,
              sellToken.decimals
            )
          : Amount.empty()

        const buyToken = fillTokens[fillTokens.length - 1]
        const buyAmount = new Amount(minBuyAmountArg)
          .divideByDecimals(buyToken.decimals)

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sellAmountFiat)

        const insufficientNativeFunds = accountNativeBalance !== ''
          ? new Amount(gasFee).gt(accountNativeBalance)
          : undefined

        return {
          ...txBase,
          status: transactionInfo.txStatus,
          fiatValue: sellAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sellAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          symbol: sellToken?.symbol ?? '',
          insufficientFundsError: accountHasInsufficientFundsForTransaction({
            accountNativeBalance,
            accountTokenBalance,
            gasFee,
            tokensList: combinedTokensList,
            tx: transactionInfo,
            account,
            nativeAsset
          }),
          insufficientFundsForGasError: insufficientNativeFunds,
          isSwap: true,
          intent: getLocale('braveWalletTransactionIntentSwap')
            .replace('$1', new Amount(normalizedTransferredValue).formatAsAsset(6, sellToken?.symbol))
            .replace('$2', buyAmount.formatAsAsset(6, buyToken.symbol))
        } as ParsedTransaction
      }

      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
      case txType === BraveWallet.TransactionType.SolanaSystemTransfer:
      case txType === BraveWallet.TransactionType.Other:
      default: {
        const sendAmountFiat = selectedNetwork
          ? computeFiatAmount(baseValue, selectedNetwork.symbol, selectedNetwork.decimals)
          : Amount.empty()

        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        return {
          ...txBase,
          status: transactionInfo.txStatus,
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork?.symbol),
          value: normalizedTransferredValue,
          valueExact: normalizedTransferredValueExact,
          symbol: selectedNetwork?.symbol ?? '',
          insufficientFundsError: accountHasInsufficientFundsForTransaction({
            accountNativeBalance,
            accountTokenBalance,
            gasFee,
            tokensList: combinedTokensList,
            tx: transactionInfo,
            account,
            nativeAsset
          }),
          insufficientFundsForGasError: accountNativeBalance !== ''
            ? new Amount(gasFee).gt(accountNativeBalance)
            : undefined,
          isSwap: to.toLowerCase() === SwapExchangeProxy,
          intent: getLocale('braveWalletTransactionIntentSend')
            .replace('$1', new Amount(normalizedTransferredValue).formatAsAsset(6, selectedNetwork?.symbol))
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
  transactionNetwork: BraveWallet.NetworkInfo
  userVisibleTokensList: BraveWallet.BlockchainToken[]
}): ParsedTransaction {
  const to = getTransactionToAddress(tx)
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const token = findTransactionToken(tx, combinedTokensList)
  const nativeAsset = makeNetworkAsset(transactionNetwork)

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
    maxPriorityFeePerGas
  } = parseTransactionFeesWithoutPrices(tx, solFeeEstimates)

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
    erc721TokenId: getTransactionErc721TokenId(tx),
    hash: tx.txHash,
    id: tx.id,
    instructions: getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData),
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
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
    token,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact
  } as ParsedTransaction
}
