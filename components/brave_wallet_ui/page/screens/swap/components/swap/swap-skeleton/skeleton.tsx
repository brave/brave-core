// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  SkeletonBox,
  SkeletonIndicator,
  Wrapper
} from './swap-skeleton.style'

export interface Props {
  width?: number
  height?: number
  borderRadius?: number
  background?: 'primary' | 'secondary'
}

export const Skeleton = (props: Props) => {
  const { width, height, borderRadius, background } = props

  return (
    <Wrapper
      width={width}
      height={height}
      borderRadius={borderRadius}
      background={background}
    >
      <SkeletonBox width={width}>
        <SkeletonIndicator />
      </SkeletonBox>
    </Wrapper>
  )
}
