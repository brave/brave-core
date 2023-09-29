// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Ring from '@brave/leo/react/progressRing'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
`

export const LoadingRing = styled(Ring)`
  --leo-progressring-size: 64px;
`
