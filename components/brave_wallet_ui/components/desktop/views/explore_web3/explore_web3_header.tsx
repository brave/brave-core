// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { ExploreNavOptions } from '../../../../options/nav-options'
import {
  SegmentedControl //
} from '../../../shared/segmented_control/segmented_control'

// components

export const ExploreWeb3Header = () => {
  // render
  return <SegmentedControl navOptions={ExploreNavOptions} />
}
