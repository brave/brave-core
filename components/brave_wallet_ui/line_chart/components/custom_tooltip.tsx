// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { TooltipContentProps } from 'recharts'

// utils
import Amount from '../../utils/amount'
import { formatTimelineDate } from '../../utils/datetime-utils'

// styles
import {
  TooltipWrapper,
  ChartBalance,
  ChartDate,
} from './custom_tooltip.styles'

type Props = Pick<
  TooltipContentProps<number, number>,
  'active' | 'coordinate' | 'label' | 'payload'
> & {
  viewBoxWidth: number
  defaultFiatCurrency: string
  hidePortfolioBalances: boolean
}

export function CustomTooltip({
  active,
  coordinate,
  label,
  payload,
  viewBoxWidth,
  defaultFiatCurrency,
  hidePortfolioBalances,
}: Props) {
  if (!active || !payload || payload.length === 0 || !payload[0].value) {
    return null
  }

  const xLeftCoordinate = Math.trunc(coordinate?.x ?? 0)
  const xRightCoordinate = xLeftCoordinate - viewBoxWidth
  const isEndOrMiddle = xRightCoordinate >= -62 ? 'end' : 'middle'
  const labelPosition = xLeftCoordinate <= 62 ? 'start' : isEndOrMiddle
  const middleEndTranslate =
    xRightCoordinate >= 8 ? 0 : Math.abs(xRightCoordinate) - 4
  const balance = new Amount(payload[0].value).formatAsFiat(defaultFiatCurrency)

  return (
    <TooltipWrapper
      labelTranslate={
        labelPosition === 'start' ? xLeftCoordinate : middleEndTranslate
      }
      labelPosition={labelPosition}
    >
      <ChartBalance>{hidePortfolioBalances ? '******' : balance}</ChartBalance>
      {label && (
        <ChartDate>{formatTimelineDate(new Date(label as number))}</ChartDate>
      )}
    </TooltipWrapper>
  )
}
