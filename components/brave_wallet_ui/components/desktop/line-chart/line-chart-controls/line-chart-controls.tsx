// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Options
import {
  ChartTimelineOptions, //
} from '../../../../options/chart-timeline-options'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  setStoredPortfolioTimeframe, //
} from '../../../../utils/local-storage-utils'

// Styled Components
import {
  ButtonMenu,
  SelectTimelinButton,
  SelectTimelinButtonIcon,
} from './line-chart-controls.style'

interface Props {
  onSelectTimeline: (id: BraveWallet.AssetPriceTimeframe) => void
  selectedTimeline: BraveWallet.AssetPriceTimeframe
}

export const LineChartControls = (props: Props) => {
  const { onSelectTimeline, selectedTimeline } = props

  // state
  const [showLineChartControlMenu, setShowLineChartControlMenu] =
    React.useState<boolean>(false)

  // methods
  const handleOnSelectTimeline = React.useCallback(
    (id: BraveWallet.AssetPriceTimeframe) => {
      onSelectTimeline(id)
      setStoredPortfolioTimeframe(id)
      setShowLineChartControlMenu(false)
    },
    [onSelectTimeline],
  )

  return (
    <ButtonMenu
      placement='bottom-end'
      isOpen={showLineChartControlMenu}
      onClose={() => setShowLineChartControlMenu(false)}
      onChange={({ isOpen }) => setShowLineChartControlMenu(isOpen)}
    >
      <SelectTimelinButton slot='anchor-content'>
        {getLocale(ChartTimelineOptions[selectedTimeline].name)}
        <SelectTimelinButtonIcon
          isOpen={showLineChartControlMenu}
          name='carat-down'
        />
      </SelectTimelinButton>
      {ChartTimelineOptions.map((option) => (
        <leo-menu-item
          key={option.id}
          onClick={() => handleOnSelectTimeline(option.id)}
        >
          {getLocale(option.name)}
        </leo-menu-item>
      ))}
    </ButtonMenu>
  )
}

export default LineChartControls
