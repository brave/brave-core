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

class ClockSettings extends React.PureComponent<Props, {}> {
  onClockFormatChanged = (selectedValue: any) => {
    this.props.actions.clockWidgetUpdated(
      this.props.showClock,
      selectedValue)
  }

  render () {
    const { toggleShowClock, showClock, clockFormat } = this.props

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
            onChange={this.onClockFormatChanged}
          >
            <div key='clock-default' data-value=''>{getLocale('clockFormatDefault')}{localeInfo}</div>
            <div key='clock-12' data-value='12'>{getLocale('clockFormat12')}</div>
            <div key='clock-24' data-value='24'>{getLocale('clockFormat24')}</div>
          </Select>
        </SettingsRow>
      </div>
    )
  }
}

export default ClockSettings
