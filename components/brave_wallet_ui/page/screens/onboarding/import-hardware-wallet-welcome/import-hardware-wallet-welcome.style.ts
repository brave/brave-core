// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import HardwareGraphicSvg from './images/hardware-graphic.svg'

export const HardwareGraphic = styled.img.attrs({
  src: HardwareGraphicSvg
})`
  width: 100%;
  height: auto;
`

export const Description = styled.p`
  color: ${leo.color.text.primary};
  text-align: center;
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 400;
  line-height: 26px;
  margin: 0;
  padding: 0;
`

export const Bold = styled.span`
  font-weight: 600;
`
