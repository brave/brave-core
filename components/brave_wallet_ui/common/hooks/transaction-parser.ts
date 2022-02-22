/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import {
  BraveWallet,
  TimeDelta,
  WalletAccountType
} from '../../constants/types'
import { MAX_UINT256 } from '../constants/magics'

// Utils
import Amount from '../../utils/amount'

// Hooks
import usePricing from './pricing'
import useAddressLabels, { SwapExchangeProxy } from './address-labels'
import useBalance from './balance'

import { getLocale } from '../../../common/locale'

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

interface ParsedTransaction extends ParsedTransactionFees {
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
}

export function useTransactionFeesParser (selectedNetwork: BraveWallet.EthereumChain, networkSpotPrice: string) {
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
    const { txDataUnion: { ethTxData1559: txData } } = transactionInfo
    const gasLimit = txData?.baseData.gasLimit || ''
    const gasPrice = txData?.baseData.gasPrice || ''
    const maxFeePerGas = txData?.maxFeePerGas || ''
    const maxPriorityFeePerGas = txData?.maxPriorityFeePerGas || ''
    const isEIP1559Transaction = maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
    const gasFee = isEIP1559Transaction
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
      missingGasLimitError: checkForMissingGasLimitError(gasLimit)
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  selectedNetwork: BraveWallet.EthereumChain,
  accounts: WalletAccountType[],
  spotPrices: BraveWallet.AssetPrice[],
  visibleTokens: BraveWallet.BlockchainToken[],
  fullTokenList?: BraveWallet.BlockchainToken[]
) {
  const { findAssetPrice, computeFiatAmount } = usePricing(spotPrices)
  const getBalance = useBalance(selectedNetwork)
  const getAddressLabel = useAddressLabels(accounts)

  const networkSpotPrice = React.useMemo(
    () => findAssetPrice(selectedNetwork.symbol),
    [selectedNetwork, findAssetPrice]
  )
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, networkSpotPrice)

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
    const { txArgs, txDataUnion: { ethTxData1559: txData }, fromAddress, txType } = transactionInfo
    const value = txData?.baseData.value || ''
    const to = txData?.baseData.to || ''
    const nonce = txData?.baseData.nonce || ''
    const account = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())
    const token = findToken(to)
    const accountNativeBalance = getBalance(account)
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

      // FIXME: swap needs a real parser to figure out the From and To details.
      case to.toLowerCase() === SwapExchangeProxy:
      case txType === BraveWallet.TransactionType.ETHSend:
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
