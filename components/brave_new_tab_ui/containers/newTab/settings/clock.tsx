// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import { Toggle } from '../../../components/toggle'
import { Select } from 'brave-ui/components'

// Utils
import { getLocale } from '../../../../common/locale'

// Types
import { useNewTabPref } from '../../../hooks/usePref'

function ClockSettings () {
    const [clockFormat, setClockFormat] = useNewTabPref('clockFormat')
    const [showClock, setShowClock] = useNewTabPref('showClock')

    let localeInfo = ''
    const dateFormat = new Intl.DateTimeFormat()
    const dateFormatOptions = dateFormat && dateFormat.resolvedOptions()
    if (dateFormatOptions && dateFormatOptions.locale) {
      localeInfo = ` (${dateFormatOptions.locale})`
    }

    return <div>
        <SettingsRow>
          <SettingsText>{getLocale('showClock')}</SettingsText>
          <Toggle
            onChange={() => setShowClock(!showClock)}
            checked={showClock}
            size='large'
          />
        </SettingsRow>
        <SettingsRow>
          <SettingsText>{getLocale('clockFormat')}</SettingsText>
          <Select
            value={clockFormat}
            onChange={setClockFormat}
          >
            <div key='clock-default' data-value=''>{getLocale('clockFormatDefault')}{localeInfo}</div>
            <div key='clock-12' data-value='12'>{getLocale('clockFormat12')}</div>
            <div key='clock-24' data-value='24'>{getLocale('clockFormat24')}</div>
          </Select>
        </SettingsRow>
      </div>
  }

export default ClockSettings
