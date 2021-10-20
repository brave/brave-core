/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  AssetPriceInfo,
  EthereumChain,
  TokenInfo,
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
  formatGasFeeFromFiat
} from '../../utils/format-balances'
import { convertMojoTimeToJS, formatDateAsRelative } from '../../utils/datetime-utils'
import usePricing from './pricing'
import useAddressLabels, { SwapExchangeProxy } from './address-labels'

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
  relativeDate: string
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
  spotPrices: AssetPriceInfo[],
  visibleTokens: TokenInfo[]
) {
  const findSpotPrice = usePricing(spotPrices)
  const getAddressLabel = useAddressLabels(accounts)
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, findSpotPrice(selectedNetwork.symbol))

  const findToken = React.useCallback((contractAddress: string) => {
    return visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }, [visibleTokens])

  return React.useCallback((transactionInfo: TransactionInfo) => {
    const { txArgs, txData } = transactionInfo
    const { baseData } = txData
    const { value, to } = baseData

    switch (true) {
      // transfer(address recipient, uint256 amount) → bool
      case transactionInfo.txType === TransactionType.ERC20Transfer: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const price = findSpotPrice(token?.symbol ?? '')
        const sendAmountFiat = formatFiatBalance(amount, token?.decimals ?? 18, price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          hash: transactionInfo.txHash,
          relativeDate: formatDateAsRelative(convertMojoTimeToJS(transactionInfo.createdTime)),
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: address,
          recipientLabel: getAddressLabel(address),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(amount, token?.decimals ?? 18),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          ...feeDetails
        } as ParsedTransaction
      }

      // approve(address spender, uint256 amount) → bool
      case transactionInfo.txType === TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = Number(gasFeeFiat).toFixed(2)

        return {
          hash: transactionInfo.txHash,
          relativeDate: formatDateAsRelative(convertMojoTimeToJS(transactionInfo.createdTime)),
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: (0).toFixed(2),
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: (0).toFixed(2),
          value: formatBalance(amount, token?.decimals ?? 18),
          symbol: token?.symbol ?? '',
          decimals: token?.decimals ?? 18,
          approvalTarget: address,
          approvalTargetLabel: getAddressLabel(address),
          ...feeDetails
        } as ParsedTransaction
      }

      // FIXME: swap needs a real parser to figure out the From and To details.
      case transactionInfo.txData.baseData.to.toLowerCase() === SwapExchangeProxy:
      case transactionInfo.txType === TransactionType.ETHSend:
      default: {
        const networkPrice = findSpotPrice(selectedNetwork.symbol)
        const sendAmountFiat = formatFiatBalance(value, selectedNetwork.decimals, networkPrice)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          hash: transactionInfo.txHash,
          relativeDate: formatDateAsRelative(convertMojoTimeToJS(transactionInfo.createdTime)),
          status: transactionInfo.txStatus,
          sender: transactionInfo.fromAddress,
          senderLabel: getAddressLabel(transactionInfo.fromAddress),
          recipient: to,
          recipientLabel: getAddressLabel(to),
          fiatValue: sendAmountFiat,
          fiatTotal: totalAmountFiat,
          nativeCurrencyTotal: formatGasFeeFromFiat(sendAmountFiat, findSpotPrice(selectedNetwork.symbol)),
          value: formatBalance(value, selectedNetwork.decimals),
          symbol: selectedNetwork.symbol,
          decimals: selectedNetwork?.decimals ?? 18,
          ...feeDetails
        } as ParsedTransaction
      }
    }
  }, [selectedNetwork, spotPrices, findToken])
}
