// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

interface Props {
  countryCode?: string
}

export const IconBox = styled.span`
  width: 24px;
  height: auto;
  display: flex;

  img {
    width: 100%;
    height: 100%;
    object-fit: contain;
  }
`

const COUNTRIES = ['AU', 'CA', 'CH', 'DE', 'ES', 'FR', 'GB', 'JP', 'NL', 'SG', 'US']

function Flag (props: Props) {
  const [url, setUrl] = React.useState<undefined | string>(undefined)

  React.useEffect(() => {
    if (COUNTRIES.includes(props?.countryCode ?? '')) {
      setUrl(`assets/country-flags/${props.countryCode}.svg`)
    }
  }, [props?.countryCode])

  return !url ? null : (
    <IconBox>
      <img src={url} />
    </IconBox>
  )
}

export default Flag
