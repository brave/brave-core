// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import theme from 'brave-ui/theme/colors/'

interface CustomReferenceDotProps {
  cx: string
  cy: number
  isAsset: boolean
  isDown: boolean
}

/**
 * This is an animated Pulsating dot that will render at the end
 * of the line chart for the current price.
 * @returns SVG Component
 */
export const CustomReferenceDot = ({
  cx, cy, isAsset, isDown
}: CustomReferenceDotProps) => {
  return (
    <>
      <circle fill='none' cx={cx} r='3' cy={cy} stroke={isAsset ? isDown ? theme.red600 : theme.teal600 : '#BF14A2'} strokeWidth='1'>
        <animate attributeName='r' values='3;8;3;3' dur='3s' begin='0s' repeatCount='indefinite' />
        <animate attributeName='opacity' values='1;0;0;0' dur='3s' begin='0s' repeatCount='indefinite' />
      </circle>
      <circle fill={isAsset ? isDown ? '#EE6374' : '#2AC194' : '#BF14A2'} cx={cx} r='3' cy={cy} />
    </>
  )
}
