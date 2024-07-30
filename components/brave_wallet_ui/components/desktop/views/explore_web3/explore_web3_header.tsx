// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'
import SegmentedControlItem from '@brave/leo/react/controlItem'

// utils
import { getLocale } from '../../../../../common/locale'
import { useLocationPathName } from '../../../../common/hooks/use-pathname'
import { ExploreNavOptions } from '../../../../options/nav-options'

// styles
import {
  SegmentedControl,
  ControlItemWrapper
} from './explore_web3_header.style'

export const ExploreWeb3Header = () => {
  // routing
  const walletLocation = useLocationPathName()
  const history = useHistory()

  return (
    <SegmentedControl
      value={walletLocation}
      onChange={({ value }) => {
        if (!value) return
        history.push(value)
      }}
    >
      {ExploreNavOptions.map((option) => (
        <SegmentedControlItem
          key={option.name}
          value={option.route}
        >
          <ControlItemWrapper>{getLocale(option.name)}</ControlItemWrapper>
        </SegmentedControlItem>
      ))}
    </SegmentedControl>
  )
}
