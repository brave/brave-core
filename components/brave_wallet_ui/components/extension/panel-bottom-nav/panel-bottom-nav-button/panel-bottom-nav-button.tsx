// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { NavOption } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { NavTooltip } from '../../../shared/nav-tooltip/nav-tooltip'

// Styled Components
import { Button, ButtonIcon } from './panel-bottom-nav-button.style'

interface Props {
  onClick: () => void
  option: NavOption
}

export const PanelBottomNavButton = (props: Props) => {
  const { onClick, option } = props

  // State
  const [active, setActive] = React.useState(false)

  // Methods
  const showTip = React.useCallback(() => {
    setActive(true)
  }, [])

  const hideTip = React.useCallback(() => {
    setActive(false)
  }, [])

  return (
    <Button onMouseEnter={showTip} onMouseLeave={hideTip} onClick={onClick}>
      <ButtonIcon name={option.icon} />

      <NavTooltip
        orientation='top'
        distance={30}
        showTip={active}
        text={getLocale(option.name)}
        horizontalAlign={
          option.id === 'buy'
            ? 'left'
            : option.id === 'activity'
              ? 'right'
              : 'center'
        }
      />
    </Button>
  )
}

export default PanelBottomNavButton
