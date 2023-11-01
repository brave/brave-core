// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { TooltipProps } from 'recharts'

// Utils
import {
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'
import Amount from '../../../../utils/amount'
import { formatTimelineDate } from '../../../../utils/datetime-utils'

// Styled Components
import { TooltipWrapper, ChartBalance, ChartDate } from '../style'

type Props = TooltipProps<number, number> & {
  onUpdateViewBoxHeight: (value: number) => void
}

function CustomTooltip({
  active,
  coordinate,
  label,
  onUpdateViewBoxHeight,
  payload,
  viewBox
}: Props) {
  // Selectors
  const defaultFiatCurrency = useSafeWalletSelector(
    WalletSelectors.defaultFiatCurrency
  )
  const hidePortfolioBalances = useSafeWalletSelector(
    WalletSelectors.hidePortfolioBalances
  )

  // Effects
  React.useLayoutEffect(() => {
    if (viewBox?.height !== undefined) {
      onUpdateViewBoxHeight(Math.trunc(viewBox.height))
    }
  }, [viewBox?.height])

  if (active && payload && payload[0].value) {
    // Computed
    const xLeftCoordinate = Math.trunc(coordinate?.x ?? 0)
    const viewBoxWidth = Math.trunc(viewBox?.width ?? 0)
    const xRightCoordinate = xLeftCoordinate - viewBoxWidth
    const isEndOrMiddle = xRightCoordinate >= -62 ? 'end' : 'middle'
    const labelPosition = xLeftCoordinate <= 62 ? 'start' : isEndOrMiddle
    const middleEndTranslate =
      xRightCoordinate >= 8 ? 0 : Math.abs(xRightCoordinate) - 4
    const balance = new Amount(payload[0].value).formatAsFiat(
      defaultFiatCurrency
    )

    return (
      <TooltipWrapper
        labelTranslate={
          labelPosition === 'start' ? xLeftCoordinate : middleEndTranslate
        }
        labelPosition={labelPosition}
      >
        <ChartBalance>
          {hidePortfolioBalances ? '******' : balance}
        </ChartBalance>
        <ChartDate>{formatTimelineDate(label)}</ChartDate>
      </TooltipWrapper>
    )
  }

  return null
}

export default CustomTooltip
