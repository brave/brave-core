/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  AssetPrice,
  EthereumChain,
  ERCToken,
  TransactionInfo,
  TransactionStatus,
  TransactionType,
  WalletAccountType
} from '../../constants/types'
import {
  formatBalance,
  formatFiatBalance,
  formatFiatGasFee,
  formatGasFee,
  formatGasFeeFromFiat,
  hexToNumber
} from '../../utils/format-balances'
import usePricing from './pricing'
import useAddressLabels, { SwapExchangeProxy } from './address-labels'
import { getLocale } from '../../../common/locale'
import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
}

interface ParsedTransaction extends ParsedTransactionFees {
  // Common fields
  hash: string
  createdTime: TimeDelta
  status: TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  fiatValue: string
  fiatTotal: string
  nativeCurrencyTotal: string
  value: string
  symbol: string
  decimals: number
  insufficientFundsError: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721ERCToken?: ERCToken
  erc721TokenId?: string
  isSwap?: boolean

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
}

export function useTransactionFeesParser (selectedNetwork: EthereumChain, networkSpotPrice: string) {
  return React.useCallback((transactionInfo: TransactionInfo): ParsedTransactionFees => {
    const { txData } = transactionInfo
    const { baseData: { gasLimit, gasPrice }, maxFeePerGas, maxPriorityFeePerGas } = txData

    const isEIP1559Transaction = maxPriorityFeePerGas !== '' && maxFeePerGas !== ''
    const gasFee = isEIP1559Transaction
      ? formatGasFee(maxFeePerGas, gasLimit, selectedNetwork.decimals)
      : formatGasFee(gasPrice, gasLimit, selectedNetwork.decimals)

    return {
      gasLimit,
      gasPrice,
      maxPriorityFeePerGas,
      maxFeePerGas,
      gasFee,
      gasFeeFiat: formatFiatGasFee(gasFee, networkSpotPrice),
      isEIP1559Transaction
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  selectedNetwork: EthereumChain,
  accounts: WalletAccountType[],
  spotPrices: AssetPrice[],
  visibleTokens: ERCToken[],
  fullTokenList?: ERCToken[]
) {
  const findSpotPrice = usePricing(spotPrices)
  const getAddressLabel = useAddressLabels(accounts)
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, findSpotPrice(selectedNetwork.symbol))

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

  return React.useCallback((transactionInfo: TransactionInfo) => {
    const { txArgs, txData, fromAddress, txType } = transactionInfo
    const { baseData } = txData
    const { value, to } = baseData
    const account = accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())
    const accountsNativeBalance = formatBalance(
      accounts.find((account) => account.address.toLowerCase() === fromAddress.toLowerCase())?.balance || '0x0',
      selectedNetwork.decimals
    )
    const usersTokenInfo = account?.tokens.find((asset) => asset.asset.contractAddress.toLowerCase() === to.toLowerCase())

    switch (true) {
      // transfer(address recipient, uint256 amount) → bool
      case txType === TransactionType.ERC20Transfer: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const price = findSpotPrice(token?.symbol ?? '')
        const sendAmount = formatBalance(amount, token?.decimals ?? 18)
        const sendAmountFiat = formatFiatBalance(amount, token?.decimals ?? 18, price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)
        const accountsTokenBalance = formatBalance(
          usersTokenInfo?.assetBalance ?? '0x0', token?.decimals ?? 18
        )
        const insufficientNativeFunds = Number(gasFee) > Number(accountsNativeBalance)
        const insufficientTokenFunds = Number(sendAmount) > Number(accountsTokenBalance)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: address,
          recipientLabel: getAddressLabel(address),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(amount, token?.decimals ?? 18),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          insufficientFundsError: insufficientNativeFunds || insufficientTokenFunds,
          contractAddressError: checkForContractAddressError(address),
          sameAddressError: checkForSameAddressError(address, fromAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      // transferFrom(address owner, address to, uint256 tokenId)
      case txType === TransactionType.ERC721TransferFrom:

      // safeTransferFrom(address owner, address to, uint256 tokenId)
      case txType === TransactionType.ERC721SafeTransferFrom: {
        // The owner of the ERC721 must not be confused with the
        // caller (fromAddress).
        const [owner, toAddress, tokenID] = txArgs
        const token = findToken(to)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = gasFeeFiat

        const insufficientNativeFunds = Number(gasFee) > Number(accountsNativeBalance)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress, // The caller, which may not be the owner
          senderLabel: getAddressLabel(fromAddress),
          recipient: toAddress,
          recipientLabel: getAddressLabel(toAddress),
          fiatValue: '0.00', // Display NFT values in the future
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(totalAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: '1', // Can only send 1 erc721 at a time
          symbol: token?.symbol ?? '',
          decimals: 0,
          insufficientFundsError: insufficientNativeFunds,
          erc721ERCToken: token,
          erc721TokenId: hexToNumber(tokenID ?? ''),
          contractAddressError: checkForContractAddressError(toAddress),
          sameAddressError: checkForSameAddressError(toAddress, owner),
          ...feeDetails
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case txType === TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = Number(gasFeeFiat).toFixed(2)
        const insufficientNativeFunds = Number(gasFee) > Number(accountsNativeBalance)
        const formattedValue = formatBalance(amount, token?.decimals ?? 18)
        const userTokenBalance = usersTokenInfo?.assetBalance ?? ''
        const allowanceValue = Number(amount) > Number(userTokenBalance) ? getLocale('braveWalletTransactionApproveUnlimited') : formattedValue

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: (0).toFixed(2),
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: (0).toFixed(2),
          value: allowanceValue,
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          approvalTarget: address,
          approvalTargetLabel: getAddressLabel(address),
          insufficientFundsError: insufficientNativeFunds,
          sameAddressError: checkForSameAddressError(address, fromAddress),
          ...feeDetails
        } as ParsedTransaction
      }

      // FIXME: swap needs a real parser to figure out the From and To details.
      case to.toLowerCase() === SwapExchangeProxy:
      case txType === TransactionType.ETHSend:
      case txType === TransactionType.Other:
      default: {
        const networkPrice = findSpotPrice(selectedNetwork.symbol)
        const sendAmount = formatBalance(value, selectedNetwork.decimals)
        const sendAmountFiat = formatFiatBalance(value, selectedNetwork.decimals, networkPrice)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat, gasFee } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          hash: transactionInfo.txHash,
          createdTime: transactionInfo.createdTime,
          status: transactionInfo.txStatus,
          sender: fromAddress,
          senderLabel: getAddressLabel(fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(value, selectedNetwork.decimals),
          symbol: selectedNetwork.symbol,
          decimals: selectedNetwork?.decimals ?? 18,
          insufficientFundsError: (Number(gasFee) + Number(sendAmount)) > Number(accountsNativeBalance),
          isSwap: to.toLowerCase() === SwapExchangeProxy,
          ...feeDetails
        } as ParsedTransaction
      }
    }
  }, [selectedNetwork, spotPrices, findToken])
}
