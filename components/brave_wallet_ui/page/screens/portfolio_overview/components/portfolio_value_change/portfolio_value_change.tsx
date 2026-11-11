// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '$wallet/constants/types'
import { TokenBalancesRegistry } from '$wallet/common/slices/entities/token-balance.entity'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import Amount from '$wallet/utils/amount'
import { getStoredPortfolioTimeframe } from '$wallet/utils/local-storage-utils'

// Hooks
import { useSyncedLocalStorage } from '$wallet/common/hooks/use_local_storage'

// Queries
import { useGetPricesHistoryQuery } from '$wallet/common/slices/api.slice'

// Components
import { LoadingSkeleton } from '$wallet/components/shared/loading-skeleton'

// Styled components
import { FiatChange, PercentBubble } from './portfolio_value_change.style'
import { Row, HorizontalSpace } from '$wallet/components/shared/style'

interface Props {
  fullPortfolioFiatBalance: Amount
  defaultFiat?: string
  tokens: BraveWallet.BlockchainToken[]
  tokenBalancesRegistry?: TokenBalancesRegistry | null
  timeframe?: BraveWallet.AssetPriceTimeframe
  skip?: boolean
}

export const PortfolioValueChange = (props: Props) => {
  const {
    fullPortfolioFiatBalance,
    defaultFiat,
    tokens,
    tokenBalancesRegistry,
    timeframe,
    skip,
  } = props

  const resolvedTimeframe = timeframe ?? getStoredPortfolioTimeframe()

  // Local Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )

  const {
    data: portfolioPriceHistory,
    isFetching: isFetchingPortfolioPriceHistory,
  } = useGetPricesHistoryQuery(
    skip || !tokens.length || !tokenBalancesRegistry || !defaultFiat
      ? skipToken
      : {
          tokens,
          timeframe: resolvedTimeframe,
          vsAsset: defaultFiat,
          tokenBalancesRegistry,
        },
  )

  const change = React.useMemo(() => {
    if (
      portfolioPriceHistory
      && portfolioPriceHistory.length !== 0
      && !fullPortfolioFiatBalance.isUndefined()
    ) {
      const oldestValue = new Amount(portfolioPriceHistory[0].close)
      return {
        difference: fullPortfolioFiatBalance.isZero()
          ? Amount.zero()
          : fullPortfolioFiatBalance.minus(oldestValue),
        oldestValue,
      }
    }

    // Case when portfolio change should not be displayed
    return {
      difference: Amount.zero(),
      oldestValue: Amount.empty(),
    }
  }, [portfolioPriceHistory, fullPortfolioFiatBalance])

  const percentageChange = React.useMemo(() => {
    const { difference, oldestValue } = change
    if (oldestValue.isUndefined()) {
      return ''
    }

    if (
      !isFetchingPortfolioPriceHistory
      && oldestValue.isZero()
      && difference.isZero()
    ) {
      return '0'
    }

    return `${difference.div(oldestValue).times(100).format(2)}`
  }, [change, isFetchingPortfolioPriceHistory])

  const fiatValueChange = React.useMemo(() => {
    const { difference, oldestValue } = change
    if (oldestValue.isUndefined()) {
      return ''
    }

    return difference.compactAsFiat(defaultFiat, 2)
  }, [defaultFiat, change])

  const isPortfolioDown = new Amount(percentageChange).lt(0)
  const fiatValueChangeDisplay = isPortfolioDown
    ? fiatValueChange
    : `+${fiatValueChange}`
  const percentageChangeDisplay = isPortfolioDown
    ? percentageChange
    : `+${percentageChange}`

  return (
    <Row
      alignItems='center'
      justifyContent='center'
      width='unset'
    >
      {fiatValueChange !== '' ? (
        <>
          <FiatChange
            textColor={isPortfolioDown ? 'error' : 'success'}
            variant='small.regular'
          >
            {hidePortfolioBalances ? '*****' : fiatValueChangeDisplay}
          </FiatChange>
          <PercentBubble isDown={isPortfolioDown}>
            {hidePortfolioBalances ? '*****' : `${percentageChangeDisplay}%`}
          </PercentBubble>
        </>
      ) : (
        <>
          <LoadingSkeleton
            width={55}
            height={24}
          />
          <HorizontalSpace space='8px' />
          <LoadingSkeleton
            width={55}
            height={24}
          />
        </>
      )}
    </Row>
  )
}
