// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../../../common/locale'

type Props = {
  show: boolean
  onCustomizeBraveToday: () => any
}

const CustomizeButton = styled('button')<Props>`
  position: fixed;
  right: 20px;
  bottom: 20px;
  appearance: none;
  cursor: pointer;
  display: block;
  border-radius: 24px;
  background: none;
  backdrop-filter: blur(25px);
  padding: 15px 34px;
  color: white;
  border: none;
  font-weight: 800;
  cursor: pointer;
  pointer-events: ${p => p.show ? 'auto' : 'none'};
  opacity: ${p => p.show ? 1 : 0};
  background: rgba(33, 37, 41, .8);
  backdrop-filter: blur(8px);
  transition: opacity 1s ease-in-out, background .124s ease-in-out;
  outline: none;
  border: none;
  &:hover {
    background: rgba(255, 255, 255, .2);
  }
  &:active {
    background: rgba(255, 255, 255, .4);
  }
  &:focus-visible {
    box-shadow: 0 0 0 1px ${p => p.theme.color.brandBrave};
  }
`

export default function Customize (props: Props) {
  return (
    <CustomizeButton onClick={props.onCustomizeBraveToday} {...props}>
      {getLocale('customize')}
    </CustomizeButton>
  )
}
