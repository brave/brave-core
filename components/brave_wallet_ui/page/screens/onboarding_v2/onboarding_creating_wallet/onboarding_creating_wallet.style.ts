// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import ProgressRing from '@brave/nala/react/progressRing'
import * as nala from '@brave/nala/tokens/css/variables'

export const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-size: 40px;
  --leo-progressring-color: ${nala.color.primitive.brands.rorange2};
`
