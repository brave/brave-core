// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Wrapper } from './swap-section-box.style'

interface Props {
  boxType: 'primary' | 'secondary'
  children?: React.ReactNode
}

export const SwapSectionBox = (props: Props) => {
  const { boxType, children } = props

  return <Wrapper boxType={boxType}>{children}</Wrapper>
}
