// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../../../common/locale'
import { Button } from '../default'

type Props = {
  show: boolean
  onCustomizeBraveToday: () => any
}

const Hideable = styled('div')<Props>`
  pointer-events: ${p => p.show ? 'auto' : 'none'};
  position: fixed;
  right: 20px;
  bottom: 20px;
  opacity: ${p => p.show ? 1 : 0};
  transition: opacity 1s ease-in-out;
`

export default function Customize (props: Props) {
  return (
    <Hideable {...props}>
      <Button
        aria-hidden={!props.show}
        disabled={!props.show}
        onClick={props.onCustomizeBraveToday}
      >
        {getLocale('customize')}
      </Button>
    </Hideable>
  )
}
