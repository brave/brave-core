// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { OpenNewIcon } from 'brave-ui/components/icons'

import createWidget from '../widget/index'
import * as Styled from './logo-style'

interface Props {
  data: NewTab.BrandedWallpaperLogo
  onClickLogo: () => void
}

function Logo ({ data, onClickLogo }: Props) {
  return (
    <>
      <Styled.Image src={data.image} alt={data.alt} />
      <Styled.Anchor href={data.destinationUrl} title={data.alt} onClick={onClickLogo}>
        <Styled.Indicator><OpenNewIcon /></Styled.Indicator>
      </Styled.Anchor>
    </>
  )
}

export default createWidget(Logo)
