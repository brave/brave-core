// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Options
import {
  ChartTimelineOptions //
} from '../../../../options/chart-timeline-options'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  setStoredPortfolioTimeframe //
} from '../../../../utils/local-storage-utils'

// Components
import {
  LineChartControlsMenu //
} from '../../wallet-menus/line-chart-controls-menu'

// Styled Components
import {
  SelectTimelinButton,
  SelectTimelineClickArea,
  SelectTimelinButtonIcon
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

  // refs
  const lineChartControlMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    lineChartControlMenuRef,
    () => setShowLineChartControlMenu(false),
    showLineChartControlMenu
  )

  // methods
  const handleOnSelectTimeline = React.useCallback(
    (id: BraveWallet.AssetPriceTimeframe) => {
      onSelectTimeline(id)
      setStoredPortfolioTimeframe(id)
      setShowLineChartControlMenu(false)
    },
    [onSelectTimeline]
  )

  return (
    <SelectTimelineClickArea ref={lineChartControlMenuRef}>
      <SelectTimelinButton
        onClick={() => setShowLineChartControlMenu((prev) => !prev)}
      >
        {getLocale(ChartTimelineOptions[selectedTimeline].name)}
        <SelectTimelinButtonIcon
          isOpen={showLineChartControlMenu}
          name='carat-down'
        />
      </SelectTimelinButton>
      {showLineChartControlMenu && (
        <LineChartControlsMenu onClick={handleOnSelectTimeline} />
      )}
    </SelectTimelineClickArea>
  )
}

export default LineChartControls
