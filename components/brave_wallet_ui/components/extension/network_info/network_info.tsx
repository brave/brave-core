// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  SectionLabel,
  SectionDetails,
} from './network_info.style'

// Shared Styles
import { Column } from '../../shared/style'

interface Props {
  network: BraveWallet.NetworkInfo
}

export const NetworkInfo = (props: Props) => {
  const { network } = props

  return (
    <StyledWrapper
      alignItems='flex-start'
      justifyContent='flex-start'
      padding='16px'
      gap='16px'
      width='100%'
    >
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_NAME)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>{network.chainName}</SectionDetails>
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_URL)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>
          {network.rpcEndpoints[0].url}
        </SectionDetails>
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_CHAIN_ID)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>{network.chainId}</SectionDetails>
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_CURRENCY_SYMBOL)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>{network.symbol}</SectionDetails>
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_WATCH_LIST_TOKEN_DECIMALS)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>{network.decimals}</SectionDetails>
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <SectionLabel textColor='tertiary'>
          {getLocale(S.BRAVE_WALLET_ALLOW_ADD_NETWORK_EXPLORER)}:
        </SectionLabel>
        <SectionDetails textColor='primary'>
          {network.blockExplorerUrls[0]}
        </SectionDetails>
      </Column>
    </StyledWrapper>
  )
}
