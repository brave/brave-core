// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import { ExploreNavOptions } from '../../../options/nav-options'

// Components
import {
  SegmentedControl //
} from '../../shared/segmented_control/segmented_control'

// Styled Components
import { HeaderWrapper } from './explorer_web3_header.style'

export const ExploreWeb3Header = () => {
  // render
  return (
    <HeaderWrapper>
      <SegmentedControl
        maxWidth='384px'
        navOptions={ExploreNavOptions}
      />
    </HeaderWrapper>
  )
}
