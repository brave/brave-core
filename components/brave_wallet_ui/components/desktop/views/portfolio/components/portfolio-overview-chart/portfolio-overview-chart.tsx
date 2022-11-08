// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { PriceDataObjectType } from '../../../../../../constants/types'

// utils
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../../../common/selectors'
import Amount from '../../../../../../utils/amount'

// components
import LineChart from '../../../../line-chart'

// style
import { Column } from '../../../../../shared/style'

interface Props {
  hasZeroBalance: boolean
  onHover: (priceAtPosition?: string | undefined) => void
}

export const PortfolioOverviewChart: React.FC<Props> = ({
  hasZeroBalance,
  onHover
}) => {
  // safe selectors
  const defaultFiatCurrency = useSafeWalletSelector(WalletSelectors.defaultFiatCurrency)
  const isFetchingPortfolioPriceHistory = useSafeWalletSelector(WalletSelectors.isFetchingPortfolioPriceHistory)

  // unsafe selectors
  const portfolioPriceHistory = useUnsafeWalletSelector(WalletSelectors.portfolioPriceHistory)

  // memos
  const priceHistory = React.useMemo((): PriceDataObjectType[] => {
    if (hasZeroBalance) {
      return []
    }
    return portfolioPriceHistory
  }, [portfolioPriceHistory, hasZeroBalance])

  // methods
  const onUpdateBalance = React.useCallback((value: number | undefined) => {
    onHover(value ? new Amount(value).formatAsFiat(defaultFiatCurrency) : undefined)
  }, [onHover, defaultFiatCurrency])

  // render
  return (
    <Column alignItems='center' fullWidth>
      <LineChart
        isDown={false}
        isAsset={false}
        priceData={priceHistory}
        onUpdateBalance={onUpdateBalance}
        isLoading={hasZeroBalance ? false : isFetchingPortfolioPriceHistory}
        isDisabled={hasZeroBalance}
      />
    </Column>
  )
}

export default PortfolioOverviewChart
