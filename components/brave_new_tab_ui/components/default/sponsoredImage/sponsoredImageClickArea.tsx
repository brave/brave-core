// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { OpenNewIcon } from 'brave-ui/components/icons'
import * as React from 'react'
import styled from 'styled-components'

const Indicator = styled.span`
  display: none;

  position: absolute;
  top: 8px;
  right: 8px;

  width: 20px;
  height: 20px;
  color: white;
`

const Link = styled.a`
  position: absolute;
  top: 8px;
  left: 8px;
  right: 56px;
  bottom: 8px;
  display: block;

  &:hover {
    cursor: pointer;
    ${Indicator} {
      display: block;
    }
  }
`

export default function SponsoredImageClickArea(props: { sponsoredImageUrl: string, onClick: () => void }) {
  return (
    <Link href={props.sponsoredImageUrl} rel="noreferrer noopener" onClick={props.onClick}>
      <Indicator>
        <OpenNewIcon />
      </Indicator>
    </Link>
  )
}
