// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useState } from 'react'

// Types / constants
import { SwapParams } from '../constants/types'
import { BraveWallet } from '../../../../constants/types'

import { MAX_UINT256 } from '../constants/magics'

// Utils
import Amount from '../../../../utils/amount'
import { hexStrToNumberArray } from '../../../../utils/hex-utils'
import { toMojoUnion } from '../../../../utils/mojo-utils'

// Query hooks
import {
  useApproveERC20AllowanceMutation,
  useGenerateSwapTransactionMutation,
  useLazyGetERC20AllowanceQuery,
  useSendEvmTransactionMutation
} from '../../../../common/slices/api.slice'

export function useZeroEx(params: SwapParams) {
  const {
    fromAccount,
    fromToken,
    fromNetwork,
    fromAmount,
    toAccountId,
    toToken,
    toAmount,
    slippageTolerance
  } = params

  // Queries
  const [getERC20Allowance] = useLazyGetERC20AllowanceQuery()

  // Mutations
  const [sendEvmTransaction] = useSendEvmTransactionMutation()
  const [approveERC20Allowance] = useApproveERC20AllowanceMutation()
  const [generateSwapTransaction] = useGenerateSwapTransactionMutation()

  // State
  const [hasAllowance, setHasAllowance] = useState<boolean>(false)

  const checkAllowance = useCallback(
    async (zeroExQuote: BraveWallet.ZeroExQuote) => {
      if (!fromAccount || !fromToken) {
        return
      }

      if (!fromToken.contractAddress) {
        setHasAllowance(true)
        return
      }

      try {
        const allowance = await getERC20Allowance({
          contractAddress: zeroExQuote.sellTokenAddress,
          ownerAddress: fromAccount.address,
          spenderAddress: zeroExQuote.allowanceTarget,
          chainId: fromToken.chainId
        }).unwrap()

        setHasAllowance(new Amount(allowance).gte(zeroExQuote.sellAmount))
      } catch (e) {
        // bubble up error
        console.log(`Error getting ERC20 allowance: ${e}`)
        setHasAllowance(false)
      }
    },
    [fromAccount, fromToken, getERC20Allowance]
  )

  const exchange = useCallback(
    async function () {
      if (
        !fromAccount ||
        !toAccountId ||
        !fromToken ||
        !toToken ||
        !fromNetwork
      ) {
        return
      }

      if (!fromAmount && !toAmount) {
        return
      }

      const fromAmountWrapped = new Amount(fromAmount)
      const toAmountWrapped = new Amount(toAmount)
      const isFromAmountEmpty =
        fromAmountWrapped.isZero() ||
        fromAmountWrapped.isNaN() ||
        fromAmountWrapped.isUndefined()
      const isToAmountEmpty =
        toAmountWrapped.isZero() ||
        toAmountWrapped.isNaN() ||
        toAmountWrapped.isUndefined()

      if (isFromAmountEmpty && isToAmountEmpty) {
        return
      }

      let transactionResponse
      try {
        transactionResponse = await generateSwapTransaction(
          toMojoUnion(
            {
              zeroExTransactionParams: {
                fromAccountId: fromAccount.accountId,
                fromChainId: fromToken.chainId,
                fromAmount: fromAmount && new Amount(fromAmount)
                  .multiplyByDecimals(fromToken.decimals)
                  .format(),
                fromToken: fromToken.contractAddress,
                toAccountId,
                toChainId: toToken.chainId,
                toAmount: toAmount && new Amount(toAmount)
                  .multiplyByDecimals(toToken.decimals)
                  .format(),
                toToken: toToken.contractAddress,
                slippagePercentage: slippageTolerance,
                routePriority: BraveWallet.RoutePriority.kRecommended
              },
              jupiterTransactionParams: undefined,
              lifiTransactionParams: undefined
            },
            'zeroExTransactionParams'
          )
        ).unwrap()
      } catch (e) {
        console.log(`Error getting 0x swap quote: ${e}`)
      }

      if (transactionResponse?.error) {
        return transactionResponse?.error
      }

      if (!transactionResponse?.response?.zeroExTransaction) {
        return
      }

      const { data, to, value, estimatedGas } =
        transactionResponse.response.zeroExTransaction

      try {
        await sendEvmTransaction({
          fromAccount,
          to,
          value: new Amount(value).toHex(),
          gas: new Amount(estimatedGas).toHex(),
          data: hexStrToNumberArray(data),
          network: fromNetwork
        })
      } catch (e) {
        // bubble up error
        console.error(`Error creating 0x transaction: ${e}`)
      }

      return undefined
    },
    [
      fromAccount,
      fromAmount,
      fromNetwork,
      fromToken,
      generateSwapTransaction,
      sendEvmTransaction,
      slippageTolerance,
      toAccountId,
      toAmount,
      toToken
    ]
  )

  const approve = useCallback(
    async (quote: BraveWallet.ZeroExQuote) => {
      if (hasAllowance) {
        return
      }

      if (!fromAccount || !fromNetwork) {
        return
      }

      const { allowanceTarget, sellTokenAddress } = quote
      try {
        await approveERC20Allowance({
          network: fromNetwork,
          fromAccount,
          contractAddress: sellTokenAddress,
          spenderAddress: allowanceTarget,

          // FIXME(onyb): reduce allowance to the minimum required amount
          // for security reasons.
          allowance: new Amount(MAX_UINT256).toHex()
        })
      } catch (e) {
        // bubble up error
        console.error(`Error creating ERC20 approve transaction: ${e}`)
      }
    },
    [approveERC20Allowance, fromAccount, fromNetwork, hasAllowance]
  )

  return {
    checkAllowance,
    hasAllowance,
    exchange,
    approve
  }
}
