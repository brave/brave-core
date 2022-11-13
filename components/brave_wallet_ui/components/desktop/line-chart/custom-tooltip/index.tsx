// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { TooltipProps } from 'recharts'

import {
  LabelWrapper,
  ChartLabel
} from '../style'

type Props = TooltipProps<number, number> & {
  onUpdateBalance: (value: number | undefined) => void
  onUpdatePosition: (value: number) => void
}

function CustomTooltip ({
  active,
  coordinate,
  label,
  onUpdateBalance,
  onUpdatePosition,
  payload,
  viewBox
}: Props) {
  const parseDate = (date: Date) => {
    const formatedDate = new Date(date).toLocaleDateString('en-US', { month: 'numeric', day: 'numeric', year: 'numeric' })
    const formatedTime = new Date(date).toLocaleTimeString('en-US', { hour: 'numeric', minute: 'numeric', hour12: true })
    return `${formatedDate} ${formatedTime}`
  }

  React.useLayoutEffect(() => {
    if (active && payload && payload.length) {
      onUpdatePosition(coordinate?.x ?? 0)
      onUpdateBalance(payload[0].value)
    }
  }, [active, payload, coordinate])

  if (active && payload && payload.length) {
    const xLeftCoordinate = Math.trunc(coordinate?.x ?? 0)
    const viewBoxWidth = Math.trunc(viewBox?.width ?? 0)
    const xRightCoordinate = xLeftCoordinate - viewBoxWidth
    const isEndOrMiddle = xRightCoordinate >= -46 ? 'end' : 'middle'
    const labelPosition = xLeftCoordinate <= 62 ? 'start' : isEndOrMiddle
    const middleEndTranslate = xRightCoordinate >= 8 ? 0 : Math.abs(xRightCoordinate) + 8

    return (
      <LabelWrapper
        labelTranslate={labelPosition === 'start' ? xLeftCoordinate : middleEndTranslate}
        labelPosition={labelPosition}
      >
        <ChartLabel>{parseDate(label)}</ChartLabel>
      </LabelWrapper>
    )
  }

  return null
}

export default CustomTooltip
