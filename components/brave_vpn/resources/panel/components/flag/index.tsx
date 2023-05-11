// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'

const FlagIcon = styled(Icon)`
  --leo-icon-size: 24px;
`

interface Props {
  countryCode?: string
}

const COUNTRIES = ['AU', 'BR', 'CA', 'CH', 'DE', 'ES', 'FR', 'GB', 'IT', 'JP', 'MX', 'NL', 'SG', 'US']

function Flag(props: Props) {
  return COUNTRIES.includes(props?.countryCode ?? '') ?
    <FlagIcon name={`country-${props.countryCode?.toLocaleLowerCase()}`} /> : null
}

export default Flag
