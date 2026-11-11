// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors } from '$wallet/common/selectors'

// Hooks
import { useSafeWalletSelector } from '$wallet/common/hooks/use-safe-selector'
import { usePortfolioAssets } from '$wallet/common/hooks/use_portfolio_assets'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import { getLocale } from '$web-common/locale'
import { useSyncedLocalStorage } from '$wallet/common/hooks/use_local_storage'
import Amount from '$wallet/utils/amount'

// Components
import { Card } from '../card'
import {
  PortfolioValueChange, //
} from '$wallet/page/screens/portfolio_overview/components/portfolio_value_change/portfolio_value_change'
import { LoadingSkeleton } from '$wallet/components/shared/loading-skeleton'

// Styles
import {
  CryptoCardBackground,
  CryptoCardIcon,
  CryptoTitle,
} from './crypto.style'
import { Column, Row, Text } from '$wallet/components/shared/style'

interface Props {
  onClick?: () => void
  onHide?: () => void
  locked?: boolean
}

export const Crypto = (props: Props) => {
  const { onClick, onHide, locked } = props

  // Selectors
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)

  // Hooks
  const {
    formattedFullPortfolioFiatBalance,
    fullPortfolioFiatBalance,
    defaultFiat,
    visibleTokensForFilteredChains,
    tokenBalancesRegistry,
  } = usePortfolioAssets()

  // Local Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )

  // Computed
  const hasZeroBalance = fullPortfolioFiatBalance.isZero()
  const zeroFiatBalance = Amount.zero().compactAsFiat(defaultFiat)
  const visibleFiatBalance =
    isWalletCreated && formattedFullPortfolioFiatBalance
      ? formattedFullPortfolioFiatBalance
      : zeroFiatBalance
  const fiatBalance = hidePortfolioBalances
    ? Amount.formatHiddenAsFiat(defaultFiat)
    : visibleFiatBalance

  return (
    <Card
      onClick={onClick}
      onHide={onHide}
      locked={locked}
    >
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
          {isWalletCreated && formattedFullPortfolioFiatBalance === '' ? (
            <LoadingSkeleton
              width={100}
              height={26}
            />
          ) : (
            <Text
              variant='heading.h3'
              textColor='primary'
            >
              {fiatBalance}
            </Text>
          )}
          {isWalletCreated && !hasZeroBalance ? (
            <PortfolioValueChange
              fullPortfolioFiatBalance={fullPortfolioFiatBalance}
              defaultFiat={defaultFiat}
              tokens={visibleTokensForFilteredChains}
              tokenBalancesRegistry={tokenBalancesRegistry}
            />
          ) : (
            !isWalletCreated && (
              <CryptoTitle variant='small.semibold'>
                {getLocale(S.BRAVE_WALLET_LETS_GET_STARTED)}
              </CryptoTitle>
            )
          )}
        </Row>
      </Column>
    </Card>
  )
}
