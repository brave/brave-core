// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import createWidget from '../widget/index'

interface Props {
  data: NewTab.BrandedWallpaperLogo
}

const LogoImage = styled('img')`
  width: 170px;
`

function Logo ({data}: Props) {
  return <a href={data.destinationUrl}>
    <LogoImage src={data.image} alt={data.alt} />
  </a>
}

export default createWidget(Logo)
