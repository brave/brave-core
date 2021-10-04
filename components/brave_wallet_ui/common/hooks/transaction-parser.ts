import * as React from 'react'

import { AssetPriceInfo, EthereumChain, TokenInfo, TransactionInfo, TransactionType } from '../../constants/types'
import { formatBalance, formatFiatBalance, formatFiatGasFee, formatGasFee } from '../../utils/format-balances'
import usePricing from './pricing'

export function useTransactionFeesParser (selectedNetwork: EthereumChain, networkSpotPrice: string) {
  return React.useCallback((transactionInfo: TransactionInfo) => {
    const { txData } = transactionInfo
    const { baseData: { gasLimit, gasPrice }, maxFeePerGas, maxPriorityFeePerGas } = txData

    const isEIP1559Transaction = maxPriorityFeePerGas && maxFeePerGas
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

export function useTransactionParser (selectedNetwork: EthereumChain, transactionSpotPrices: AssetPriceInfo[], visibleTokens: TokenInfo[]) {
  const findSpotPrice = usePricing(transactionSpotPrices)
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, findSpotPrice(selectedNetwork.symbol))

  const findToken = React.useCallback((contractAddress: string) => {
    return visibleTokens.find((token) => token.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }, [visibleTokens])

  return React.useCallback((transactionInfo: TransactionInfo) => {
    const { txType, txArgs, txData } = transactionInfo
    const { baseData } = txData
    const { value, to } = baseData

    switch (txType) {
      // transfer(address recipient, uint256 amount) → bool
      case TransactionType.ERC20Transfer:

      // approve(address spender, uint256 amount) → bool
      case TransactionType.ERC20Approve: {
        const [address, amount] = txArgs
        const token = findToken(to)
        const price = findSpotPrice(token?.symbol ?? '')
        const sendAmountFiat = formatFiatBalance(amount, token?.decimals ?? 18, price)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          sendAmount: formatBalance(amount, token?.decimals ?? 18),
          sendAmountFiat,
          sendTo: address,
          symbol: token?.symbol ?? '',
          totalAmountFiat,
          ...feeDetails
        }
      }

      case TransactionType.ETHSend:
      default: {
        const networkPrice = findSpotPrice(selectedNetwork.symbol)
        const sendAmountFiat = formatFiatBalance(value, selectedNetwork.decimals, networkPrice)

        const feeDetails = parseTransactionFees(transactionInfo)
        const { gasFeeFiat } = feeDetails
        const totalAmountFiat = (Number(gasFeeFiat) + Number(sendAmountFiat)).toFixed(2)

        return {
          sendAmount: formatBalance(value, selectedNetwork.decimals),
          sendAmountFiat,
          sendTo: to,
          symbol: selectedNetwork.symbol,
          totalAmountFiat,
          ...feeDetails
        }
      }
    }
  }, [selectedNetwork, transactionSpotPrices, findToken])
}
