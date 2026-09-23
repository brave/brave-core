// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import { getLocale } from '$web-common/locale'
import { useSyncedLocalStorage } from '$wallet/common/hooks/use_local_storage'

// Components
import { Card } from '../card'

// Styles
import {
  CryptoCardBackground,
  CryptoCardIcon,
  CryptoTitle,
} from './crypto.style'
import { Column, Row, Text } from '$wallet/components/shared/style'

interface Props {
  onClick?: () => void
}

export const Crypto = (props: Props) => {
  const { onClick } = props

  // Local Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )

  return (
    <Card onClick={onClick}>
      <CryptoCardBackground />
      <Column
        justifyContent='flex-start'
        alignItems='flex-start'
        padding='8px 16px'
        width='100%'
        gap='8px'
      >
        <Row
          gap='8px'
          justifyContent='flex-start'
          alignItems='center'
        >
          <CryptoCardIcon name='crypto-wallets' />
          <Text
            variant='default.semibold'
            textColor='primary'
          >
            {getLocale(S.BRAVE_WALLET_CRYPTO)}
          </Text>
        </Row>
        <Row
          padding='0px 0px 0px 28px'
          justifyContent='space-between'
          alignItems='center'
          gap='8px'
        >
          <Text
            variant='heading.h3'
            textColor='primary'
          >
            {hidePortfolioBalances ? '$••••' : '$0.00'}
          </Text>
          <CryptoTitle variant='small.semibold'>
            {getLocale(S.BRAVE_WALLET_LETS_GET_STARTED)}
          </CryptoTitle>
        </Row>
      </Column>
    </Card>
  )
}
