// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import { ChartTimelineOptions } from '../../../options/chart-timeline-options'

// Utils
import { getLocale } from '../../../../common/locale'

// Types
import { BraveWallet } from '../../../constants/types'

// Styled Components
import {
  LineChartWrapper,
  PopupButton,
  PopupButtonText
} from './wellet-menus.style'

export interface Props {
  onClick: (id: BraveWallet.AssetPriceTimeframe) => void
}

export const LineChartControlsMenu = (props: Props) => {
  const { onClick } = props

  return (
    <LineChartWrapper yPosition={32}>
      {ChartTimelineOptions.map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option.id)}
          minWidth={130}
        >
          <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
        </PopupButton>
      ))}
    </LineChartWrapper>
  )
}
