// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

interface Props {
  className?: string
}

const IconSVG = styled('svg')`
  fill: currentColor;
  width: 100%;
  height: 100%;
`

export default function EllipsisIcon (props: Props) {
  return (
    <IconSVG className={props.className} xmlns={'http://www.w3.org/2000/svg'} viewBox="0 0 24 24">
      <path d={'M18 14.25a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5zm-6 0a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5zm-6 0a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5z'} fillRule={'evenodd'}/>
    </IconSVG>
  )
}
