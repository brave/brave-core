// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useState } from 'react'

// types
import { BraveWallet } from '../../constants/types'

// magics
import { MAX_UINT256 } from '../constants/magics'

// utils
import Amount from '../../utils/amount'

// api
import {
  useLazyGetERC20AllowanceQuery,
  useApproveERC20AllowanceMutation
} from '../slices/api.slice'

export function useTokenAllowance() {
  // Queries
  const [getERC20Allowance] = useLazyGetERC20AllowanceQuery()

  // Mutations
  const [approveERC20Allowance] = useApproveERC20AllowanceMutation()

  // State
  const [hasAllowance, setHasAllowance] = useState<boolean>(false)

  // Methods
  const checkAllowance = useCallback(
    async ({
      spenderAddress,
      spendAmount,
      account,
      token
    }: {
      spenderAddress: string
      spendAmount: string
      account: BraveWallet.AccountInfo
      token: BraveWallet.BlockchainToken
    }) => {
      if (!token.contractAddress || token.coin !== BraveWallet.CoinType.ETH) {
        setHasAllowance(true)
        return
      }

      try {
        const allowance = await getERC20Allowance({
          contractAddress: token.contractAddress,
          ownerAddress: account.address,
          spenderAddress: spenderAddress,
          chainId: token.chainId
        }).unwrap()

        setHasAllowance(new Amount(allowance).gte(spendAmount))
      } catch (e) {
        // bubble up error
        console.log(`Error getting ERC20 allowance: ${e}`)
        setHasAllowance(false)
      }
    },
    [getERC20Allowance]
  )

  const approveSpendAllowance = useCallback(
    async ({
      spenderAddress,
      spendAmount,
      account,
      network,
      token
    }: {
      spenderAddress: string
      spendAmount?: string
      account: BraveWallet.AccountInfo
      network: BraveWallet.NetworkInfo
      token: BraveWallet.BlockchainToken
    }) => {
      if (hasAllowance) {
        return
      }

      try {
        await approveERC20Allowance({
          network: network,
          fromAccount: account,
          contractAddress: token.contractAddress,
          spenderAddress,
          allowance: spendAmount
            ? new Amount(spendAmount).toHex()
            : new Amount(MAX_UINT256).toHex()
        })
      } catch (e) {
        // bubble up error
        console.error(`Error creating ERC20 approve transaction: ${e}`)
      }
    },
    [approveERC20Allowance, hasAllowance]
  )

  return {
    checkAllowance,
    hasAllowance,
    approveSpendAllowance
  }
}
