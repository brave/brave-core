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
import * as newTabActions from '../../../actions/new_tab_actions'

interface Props {
  actions: typeof newTabActions
  toggleShowClock: () => void
  showClock: boolean
  clockFormat: string
}

function useDarkPreference (): boolean {
  const mql = window.matchMedia('(prefers-color-scheme: dark)')
  const [isDark, setDark] = React.useState(mql.matches)

  React.useEffect(() => {
    const handleChange = () => setDark(mql.matches)
    mql.addEventListener('change', handleChange)
    return () => mql.removeEventListener('change', handleChange)
  }, [])

  return isDark
}

function ClockSettings ({
  actions,
  toggleShowClock,
  showClock,
  clockFormat
}: Props) {
  const isDarkMode = useDarkPreference()
  const onClockFormatChanged = (selectedValue: any) => {
    actions.clockWidgetUpdated(showClock, selectedValue)
  }

  let localeInfo = ''
  const dateFormat = new Intl.DateTimeFormat()
  const dateFormatOptions = dateFormat && dateFormat.resolvedOptions()
  if (dateFormatOptions && dateFormatOptions.locale) {
    localeInfo = ` (${dateFormatOptions.locale})`
  }

  return (
    <div>
      <SettingsRow>
        <SettingsText>{getLocale('showClock')}</SettingsText>
        <Toggle
          onChange={toggleShowClock}
          checked={showClock}
          size='large'
        />
      </SettingsRow>
      <SettingsRow>
        <SettingsText>{getLocale('clockFormat')}</SettingsText>
        <Select
          value={clockFormat}
          onChange={onClockFormatChanged}
          type={isDarkMode ? 'dark' : 'light'}
        >
          <div key='clock-default' data-value=''>{getLocale('clockFormatDefault')}{localeInfo}</div>
          <div key='clock-12' data-value='12'>{getLocale('clockFormat12')}</div>
          <div key='clock-24' data-value='24'>{getLocale('clockFormat24')}</div>
        </Select>
      </SettingsRow>
    </div>
  )
}

export default ClockSettings
