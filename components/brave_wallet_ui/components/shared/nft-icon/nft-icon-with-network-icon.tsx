// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// components
import CreateNetworkIcon from '../create-network-icon'
import { NftIcon, NftIconProps } from './nft-icon'

// styles
import { NetworkIconWrapper } from '../../../page/screens/send/components/token-list-item/token-list-item.style'
import { IconWrapper } from './nft-icon-styles'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

interface Props extends NftIconProps {
  size?: string | number
  chainId?: string
  coinType?: BraveWallet.CoinType
  disabled?: boolean
}

export const NftIconWithNetworkIcon = (props: Props) => {
  const { chainId, coinType, disabled } = props

  const { data: network } = useGetNetworkQuery(
    coinType !== undefined && chainId !== undefined
      ? { chainId: chainId, coin: coinType }
      : undefined,
    { skip: coinType === undefined || chainId === undefined }
  )

  return (
    <>
      <NftIcon {...props} />
      <IconWrapper disabled={disabled}>
        <NetworkIconWrapper>
          <CreateNetworkIcon network={network} marginRight={0} />
        </NetworkIconWrapper>
      </IconWrapper>
    </>
  )
}
