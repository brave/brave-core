// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'

import {
  useGetHardwareAccountDiscoveryBalanceQuery //
} from '../../../common/slices/api.slice'
import { useAddressOrb } from '../../../common/hooks/use-orb'

// Styles
import { Skeleton } from '../../shared/loading-skeleton/styles'

// Styles
import {
  HardwareWalletAccountCircle,
  HardwareWalletAccountListItem,
  HardwareWalletAccountListItemRow,
  AddressBalanceWrapper,
  AccountCheckbox
} from './hardware_wallet_connect.styles'

interface AccountListItemProps {
  address: string
  onSelect: () => void
  selected: boolean
  disabled: boolean
  balanceAsset: Pick<
    BraveWallet.BlockchainToken,
    | 'coin'
    | 'chainId'
    | 'contractAddress'
    | 'isErc721'
    | 'isNft'
    | 'symbol'
    | 'tokenId'
    | 'decimals'
  >
}

export const AccountListItem = ({
  address,
  onSelect,
  selected,
  disabled,
  balanceAsset
}: AccountListItemProps) => {
  // queries
  const { data: balanceResult, isFetching: isLoadingBalance } =
    useGetHardwareAccountDiscoveryBalanceQuery({
      coin: balanceAsset.coin,
      chainId: balanceAsset.chainId,
      address: address
    })

  // memos
  const orb = useAddressOrb(address)

  const balance = React.useMemo(() => {
    if (
      isLoadingBalance ||
      !balanceResult ||
      balanceAsset.decimals === undefined
    ) {
      return undefined
    }

    return new Amount(balanceResult)
      .divideByDecimals(balanceAsset.decimals)
      .formatAsAsset(undefined, balanceAsset.symbol)
  }, [
    isLoadingBalance,
    balanceResult,
    balanceAsset.decimals,
    balanceAsset.symbol
  ])

  // render
  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemRow>
        <AddressBalanceWrapper>
          <Tooltip
            mode='default'
            placement='top'
          >
            <div>{reduceAddress(address)}</div>
          </Tooltip>
        </AddressBalanceWrapper>
        {isLoadingBalance ? (
          <Skeleton
            width={'140px'}
            height={'100%'}
          />
        ) : (
          <AddressBalanceWrapper>{balance}</AddressBalanceWrapper>
        )}
        <AccountCheckbox
          checked={selected}
          onChange={onSelect}
          isDisabled={disabled}
        >
          <div data-key={'selected'} />
        </AccountCheckbox>
      </HardwareWalletAccountListItemRow>
    </HardwareWalletAccountListItem>
  )
}
