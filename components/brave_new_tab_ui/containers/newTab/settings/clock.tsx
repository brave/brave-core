// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import Toggle from '@brave/leo/react/toggle'
import Dropdown from '@brave/leo/react/dropdown'

// Utils
import { getLocale } from '../../../../common/locale'

// Types
import { useNewTabPref } from '../../../hooks/usePref'
import styled from 'styled-components'

const StyledDropdown = styled(Dropdown)`
  display: block;
  width: 220px;
`

function ClockSettings() {
  const [clockFormat, setClockFormat] = useNewTabPref('clockFormat')
  const [showClock, setShowClock] = useNewTabPref('showClock')

  const dateFormat = new Intl.DateTimeFormat()
  const dateFormatOptions = dateFormat && dateFormat.resolvedOptions()
  const defaultClockFormatText = getLocale('clockFormatDefault') + (dateFormatOptions && dateFormatOptions.locale
    ? ` (${dateFormatOptions.locale})`
    : '')

  return <div>
    <SettingsRow>
      <SettingsText>{getLocale('showClock')}</SettingsText>
      <Toggle
        onChange={() => setShowClock(!showClock)}
        checked={showClock}
        size='small'
      />
    </SettingsRow>
    {showClock && <SettingsRow>
      <SettingsText>{getLocale('clockFormat')}</SettingsText>
      <StyledDropdown value={clockFormat} onChange={e => setClockFormat(e.value!)} positionStrategy='fixed'>
        <span slot="value">
          {clockFormat
            ? getLocale('clockFormat' + clockFormat)
            : defaultClockFormatText}
        </span>
        <leo-option value=''>{defaultClockFormatText}</leo-option>
        <leo-option value='12'>{getLocale('clockFormat12')}</leo-option>
        <leo-option value='24'>{getLocale('clockFormat24')}</leo-option>
      </StyledDropdown>
    </SettingsRow>}
  </div>
}

export default ClockSettings
