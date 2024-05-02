// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import HardwareGraphicLightSvg from './images/hardware_graphic_light.svg'
import HardwareGraphicDarkSvg from './images/hardware_graphic_dark.svg'

export const HardwareGraphic = styled.img.attrs({
  src: window.matchMedia('(prefers-color-scheme: dark)').matches
    ? HardwareGraphicDarkSvg
    : HardwareGraphicLightSvg
})`
  width: 100%;
  height: auto;
  margin: 98px 0 40px;
`

export const Description = styled.p`
  color: ${leo.color.text.primary};
  text-align: center;
  font: ${leo.font.large.regular};
  font-size: 16px;
  margin: 0;
  padding: 0;
`

export const Bold = styled.span`
  font-weight: 600;
`
