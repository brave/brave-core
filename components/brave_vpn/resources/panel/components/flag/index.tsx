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
  countryCode: string
}

// when adding a new entry here, please also add SVG for the entry into
// `brave/ui/webui/resources/BUILD.gn` (under `enable_brave_vpn_panel`)
const COUNTRIES = [
    'AL',
    'AR',
    'AT',
    'AU',
    'BE',
    'BG',
    'BR',
    'CA',
    'CH',
    'CL',
    'CO',
    'CR',
    'CY',
    'CZ',
    'DE',
    'DK',
    'EE',
    'ES',
    'FI',
    'FR',
    'GB',
    'GR',
    'HK',
    'HR',
    'HU',
    'ID',
    'IE',
    'IS',
    'IT',
    'JP',
    'KR',
    'LU',
    'MX',
    'MY',
    'NL',
    'NO',
    'NZ',
    'PA',
    'PE',
    'PL',
    'PT',
    'PY',
    'RO',
    'SE',
    'SG',
    'TH',
    'TR',
    'UA',
    'US',
    'UY',
    'ZA',
    `WORLDWIDE`
]

function Flag(props: Props) {
  return COUNTRIES.includes(props.countryCode) ?
    <FlagIcon name={`country-${props.countryCode?.toLocaleLowerCase()}`} /> : null
}

export default Flag
