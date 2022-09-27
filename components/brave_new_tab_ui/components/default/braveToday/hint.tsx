// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

interface Props {
  color: string
}

const Hint = styled('div')<Props>`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  text-align: center;
  font-size: 15px;
  color: ${p => p.color};
  > p {
    margin: 0;
  }
`

const Graphic = styled('div')`
  width: 16px;
  height: 16px;
`

export default function BraveTodayHint (props: Props) {
  return (
    <Hint color={props.color}>
      <p>{getLocale('braveTodayScrollHint')}</p>
      <Graphic>
        <CaratStrongDownIcon />
      </Graphic>
    </Hint>
  )
}
