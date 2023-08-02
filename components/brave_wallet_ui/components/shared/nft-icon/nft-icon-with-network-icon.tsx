// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { CoinType } from '../../../constants/types'

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
  coinType?: CoinType
  disabled?: boolean
  hideNetworkIcon?: boolean
}

export const NftIconWithNetworkIcon = (props: Props) => {
  const { chainId, coinType, disabled, hideNetworkIcon } = props

  const { data: network } = useGetNetworkQuery(
    coinType !== undefined && chainId !== undefined
      ? { chainId: chainId, coin: coinType }
      : skipToken
  )

  return (
    <>
      <NftIcon {...props} />
      {!hideNetworkIcon &&
        <IconWrapper disabled={disabled}>
          <NetworkIconWrapper>
            <CreateNetworkIcon network={network} marginRight={0} />
          </NetworkIconWrapper>
        </IconWrapper>
      }
    </>
  )
}
