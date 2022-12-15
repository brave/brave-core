// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

const Loading = styled('div')`
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
`

const Graphic = styled('div')`
  width: 50px;
  height: 50px;
  align-self: center;

  svg {
    fill: white;
  }
`

export default function Loader () {
  return (
    <Loading aria-busy='true'>
      <Graphic aria-label='Loading'>
        <LoaderIcon />
      </Graphic>
    </Loading>
  )
}
