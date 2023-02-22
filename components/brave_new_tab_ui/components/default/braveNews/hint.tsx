// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

const Hint = styled('div')<{}>`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  text-align: center;
  font-size: 15px;
  color: var(--override-readability-color, #FFFFFF);
  > p {
    margin: 0;
  }
`

const Graphic = styled('div')`
  width: 16px;
  height: 16px;
`

export default function BraveNewsHint () {
  return (
    <Hint>
      <p>{getLocale('braveNewsScrollHint')}</p>
      <Graphic>
        <CaratStrongDownIcon />
      </Graphic>
    </Hint>
  )
}
