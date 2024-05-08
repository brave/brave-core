// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as leo from '@brave/leo/tokens/css/variables'
import { Dot, DotProps } from 'recharts'

type Props = DotProps & {
  onUpdateYPosition: (value: number) => void
  onUpdateXPosition: (value: number) => void
}

export const CustomReferenceDot = ({
  cx,
  cy,
  onUpdateYPosition,
  onUpdateXPosition
}: Props) => {
  // Effects
  React.useLayoutEffect(() => {
    if (cy !== undefined && cx !== undefined) {
      onUpdateYPosition(cy + 10)
      onUpdateXPosition(cx)
    }
  }, [cy, cx, onUpdateYPosition, onUpdateXPosition])

  return (
    <Dot
      stroke={leo.color.icon.interactive}
      fill={leo.color.white}
      strokeWidth={2}
      r={5}
      cx={cx}
      cy={cy}
    />
  )
}
