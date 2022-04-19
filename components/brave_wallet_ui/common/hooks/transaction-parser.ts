/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import {
  BraveWallet,
  SolFeeEstimates,
  TimeDelta,
  WalletAccountType
} from '../../constants/types'
import { MAX_UINT256 } from '../constants/magics'

// Utils
import Amount from '../../utils/amount'
import { getLocale } from '../../../common/locale'

// Hooks
import usePricing from './pricing'
import useAddressLabels, { SwapExchangeProxy } from './address-labels'
import useBalance from './balance'

// Options
import { makeNetworkAsset } from '../../options/asset-options'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
  missingGasLimitError?: string
}

export interface ParsedTransaction extends ParsedTransactionFees {
  // Common fields
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
  decimals: number
  insufficientFundsError: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721BlockchainToken?: BraveWallet.BlockchainToken
  erc721TokenId?: string
  isSwap?: boolean

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: string
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: string
}

export function useTransactionFeesParser (selectedNetwork: BraveWallet.NetworkInfo, networkSpotPrice: string, solFeeEstimates?: SolFeeEstimates) {
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
  const checkForMissingGasLimitError = React.useCallback((gasLimit: string): string | undefined => {
    return (gasLimit === '' || Amount.normalize(gasLimit) === '0')
      ? getLocale('braveWalletMissingGasLimitError')
      : undefined
  }, [])

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo): ParsedTransactionFees => {
    const { txDataUnion: { ethTxData1559: txData, filTxData }, txType } = transactionInfo
    const isSolTransaction =
      txType === BraveWallet.TransactionType.SolanaSystemTransfer ||
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
    const isFilTrtansaction = filTxData !== undefined
    const gasLimit = isFilTrtansaction ? filTxData.gasLimit : txData?.baseData.gasLimit || ''
    const gasPrice = txData?.baseData.gasPrice || ''
    const maxFeePerGas = txData?.maxFeePerGas || ''
    const maxPriorityFeePerGas = txData?.maxPriorityFeePerGas || ''
    const isEIP1559Transaction = maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
    const gasFee = isSolTransaction
      ? new Amount(solFeeEstimates?.fee.toString() ?? '').format()
      : isEIP1559Transaction
        ? new Amount(maxFeePerGas)
          .times(gasLimit)
          .format()
        : new Amount(gasPrice)
          .times(gasLimit)
          .format()

    return {
      gasLimit: Amount.normalize(gasLimit),
      gasPrice: Amount.normalize(gasPrice),
      maxFeePerGas: Amount.normalize(maxFeePerGas),
      maxPriorityFeePerGas: Amount.normalize(maxPriorityFeePerGas),
      gasFee,
      gasFeeFiat: new Amount(gasFee)
        .divideByDecimals(selectedNetwork.decimals)
        .times(networkSpotPrice)
        .formatAsFiat(),
      isEIP1559Transaction,
      missingGasLimitError: isSolTransaction ? undefined : checkForMissingGasLimitError(gasLimit)
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  selectedNetwork: BraveWallet.NetworkInfo,
  accounts: WalletAccountType[],
  spotPrices: BraveWallet.AssetPrice[],
  visibleTokens: BraveWallet.BlockchainToken[],
  fullTokenList?: BraveWallet.BlockchainToken[],
  solFeeEstimates?: SolFeeEstimates
) {
  const nativeAsset = React.useMemo(
    () => makeNetworkAsset(selectedNetwork),
    [selectedNetwork]
  )
  const { findAssetPrice, computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance([selectedNetwork])
  const getAddressLabel = useAddressLabels(accounts)

  const networkSpotPrice = React.useMemo(
    () => findAssetPrice(selectedNetwork.symbol),
    [selectedNetwork, findAssetPrice]
  )
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, networkSpotPrice, solFeeEstimates)

  const findToken = React.useCallback((contractAddress: string) => {
    const checkVisibleList = visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase())
    return checkVisibleList ?? (fullTokenList?.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase()))
  }, [visibleTokens, fullTokenList])

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
   *
   * @param to - The address to check
   * @returns Localized string describing the error, or undefined in case of
   * no error.
   */
  const checkForContractAddressError = (to: string): string | undefined => {
    return fullTokenList?.some(token => token.contractAddress.toLowerCase() === to.toLowerCase())
      ? getLocale('braveWalletContractAddressError')
      : undefined
  }

  /**
   * Checks if a given set of sender and recipient addresses are the
   * same.
   *
   * @remarks
   *
   * This function must only be used for the following transaction types:
   *  - ERC20Transfer
   *  - ERC721TransferFrom
   *  - ERC721SafeTransferFrom
   *  - ERC20Approve
   *  - ETHSend
   *
   * @param to - The recipient address
   * @param from - The sender address
   */
  const checkForSameAddressError = (to: string, from: string): string | undefined => {
    return to.toLowerCase() === from.toLowerCase()
      ? getLocale('braveWalletSameAddressError')
      : undefined
  }

  return React.useCallback((transactionInfo: BraveWallet.TransactionInfo) => {
    const { txArgs, txDataUnion: { ethTxData1559: txData, solanaTxData: solTxData }, fromAddress, txType } = transactionInfo
    const isSPLTransaction =
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
      txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
    const isSolTransaction =
      txType === BraveWallet.TransactionType.SolanaSystemTransfer ||
      isSPLTransaction
    const value = isSPLTransaction
      ? solTxData?.amount.toString() ?? ''
      : isSolTransaction
        ? solTxData?.lamports.toString() ?? ''
        : txData?.baseData.value || ''
    const to = isSolTransaction
      ? solTxData?.toWalletAddress ?? ''
      : txData?.baseData.to || ''
    const nonce = txData?.baseData.nonce || ''
    const account = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())
    const token = isSPLTransaction ? findToken(solTxData?.splTokenMintAddress ?? '') : findToken(to)
    const accountNativeBalance = getBalance(account, nativeAsset)
    const accountTokenBalance = getBalance(account, token)

    switch (true) {
      // transfer(address recipient, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Transfer: {
        const [address, amount] = txArgs
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(amount)
          .divideByDecimals(token?.decimals ?? 18)
          .times(price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = new Amount(gasFee)
          .gt(accountNativeBalance)
        const insufficientTokenFunds = new Amount(amount)
          .gt(accountTokenBalance)

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: address,
          recipientLabel: getAddressLabel(address),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork.symbol),
          value: new Amount(amount)
            .divideByDecimals(token?.decimals ?? 18)
            .format(6),
          valueExact: new Amount(amount)
            .divideByDecimals(token?.decimals ?? 18)
            .format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          insufficientFundsError: insufficientNativeFunds || insufficientTokenFunds,
          contractAddressError: checkForContractAddressError(address),
          sameAddressError: checkForSameAddressError(address, fromAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === BraveWallet.TransactionType.ERC721SafeTransferFrom: {
        // The owner of the ERC721 must not be confused with the
        // caller (fromAddress).
        const [owner, toAddress, tokenID] = txArgs

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = gasFeeFiat

        const insufficientNativeFunds = new Amount(gasFee)
          .gt(accountNativeBalance)

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress, // The caller, which may not be the owner
          senderLabel: getAddressLabel(fromAddress),
          recipient: toAddress,
          recipientLabel: getAddressLabel(toAddress),
          fiatValue: Amount.zero(), // Display NFT values in the future
          fiatTotal: new Amount(totalAmountFiat),
          formattedNativeCurrencyTotal: totalAmountFiat && new Amount(totalAmountFiat)
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork.symbol),
          value: '1', // Can only send 1 erc721 at a time
          valueExact: '1',
          symbol: token?.symbol ?? '',
          decimals: 0,
          insufficientFundsError: insufficientNativeFunds,
          erc721BlockchainToken: token,
          erc721TokenId: tokenID && `#${Amount.normalize(tokenID)}`,
          contractAddressError: checkForContractAddressError(toAddress),
          sameAddressError: checkForSameAddressError(toAddress, owner),
          ...feeDetails
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === BraveWallet.TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = new Amount(gasFeeFiat)
        const insufficientNativeFunds = new Amount(gasFee)
          .gt(accountNativeBalance)

        const amountWrapped = new Amount(amount)

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: Amount.zero(),
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: Amount.zero()
            .formatAsAsset(2, selectedNetwork.symbol),
          value: amountWrapped
            .divideByDecimals(token?.decimals ?? 18)
            .format(6),
          valueExact: amountWrapped
            .divideByDecimals(token?.decimals ?? 18)
            .format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          approvalTarget: address,
          approvalTargetLabel: getAddressLabel(address),
          isApprovalUnlimited: amountWrapped.eq(MAX_UINT256),
          insufficientFundsError: insufficientNativeFunds,
          sameAddressError: checkForSameAddressError(address, fromAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer:
      case txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation: {
        const price = findAssetPrice(token?.symbol ?? '')
        const sendAmountFiat = new Amount(value)
          .divideByDecimals(token?.decimals ?? 9)
          .times(price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        const insufficientNativeFunds = new Amount(gasFee)
          .gt(accountNativeBalance)
        const insufficientTokenFunds = new Amount(value)
          .gt(accountTokenBalance)

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork.symbol),
          value: new Amount(value)
            .divideByDecimals(token?.decimals ?? 9)
            .format(6),
          valueExact: new Amount(value)
            .divideByDecimals(token?.decimals ?? 9)
            .format(),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 9,
          insufficientFundsError: insufficientNativeFunds || insufficientTokenFunds,
          contractAddressError: checkForContractAddressError(solTxData?.toWalletAddress ?? ''),
          sameAddressError: checkForSameAddressError(solTxData?.toWalletAddress ?? '', fromAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      // args: (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
      case txType === BraveWallet.TransactionType.ETHSwap: {
        const [fillPath, sellAmountArg, minBuyAmountArg] = txArgs
        const fillContracts = fillPath
          .slice(2)
          .match(/.{1,40}/g)
        const fillTokens = (fillContracts || [])
          .map(path => '0x' + path)
          .map(address =>
            address === '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee'
              ? nativeAsset
              : findToken(address) || nativeAsset)

        const sellToken = fillTokens.length === 1
          ? nativeAsset
          : fillTokens[0]
        const sellAmountWeiBN = new Amount(sellAmountArg || value)
        const sellAmountBN = sellAmountWeiBN
          .divideByDecimals(sellToken.decimals)
        const sellAmountFiat = computeFiatAmount(
          sellAmountBN.format(),
          sellToken.symbol,
          sellToken.decimals
        )

        const buyToken = fillTokens[fillTokens.length - 1]
        const buyAmount = new Amount(minBuyAmountArg)
          .divideByDecimals(buyToken.decimals)
          .format(6)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sellAmountFiat)

        const insufficientNativeFunds = new Amount(gasFee)
          .gt(accountNativeBalance)
        const insufficientTokenFunds = sellAmountBN
          .gt(getBalance(account, token))

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sellAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sellAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork.symbol),
          value: sellAmountBN.format(6),
          valueExact: sellAmountBN.format(),
          symbol: sellToken.symbol,
          decimals: sellToken.decimals,
          insufficientFundsError: insufficientNativeFunds || insufficientTokenFunds,

          // Set isSwap=false to differentiate ETHSwap from SwapExchangeProxy
          // case.
          isSwap: false,
          sellToken,
          sellAmount: sellAmountBN
            .format(6),
          buyToken,
          minBuyAmount: buyAmount,
          ...feeDetails
        } as ParsedTransaction
      }

      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
      case txType === BraveWallet.TransactionType.SolanaSystemTransfer:
      case txType === BraveWallet.TransactionType.Other:
      default: {
        const sendAmountFiat = computeFiatAmount(value, selectedNetwork.symbol, selectedNetwork.decimals)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = new Amount(gasFeeFiat)
          .plus(sendAmountFiat)

        return {
          hash: transactionInfo.txHash,
          nonce,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          formattedNativeCurrencyTotal: sendAmountFiat
            .div(networkSpotPrice)
            .formatAsAsset(6, selectedNetwork.symbol),
          value: new Amount(value)
            .divideByDecimals(selectedNetwork.decimals)
            .format(6),
          valueExact: new Amount(value)
            .divideByDecimals(selectedNetwork.decimals)
            .format(),
          symbol: selectedNetwork.symbol,
          decimals: selectedNetwork?.decimals ?? 18,
          insufficientFundsError: new Amount(value)
            .plus(gasFee)
            .gt(accountNativeBalance),
          isSwap: to.toLowerCase() === SwapExchangeProxy,
          ...feeDetails
        } as ParsedTransaction
      }
    }
  }, [selectedNetwork, accounts, spotPrices, findToken])
}
