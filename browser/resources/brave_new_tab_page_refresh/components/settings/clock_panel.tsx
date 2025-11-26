/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Toggle from '@brave/leo/react/toggle'

import { useNewTabState, useNewTabActions } from '../../context/new_tab_context'
import { ClockFormat } from '../../state/new_tab_state'
import { getString } from '../../lib/strings'
import { formatString } from '$web-common/formatString'

import { style } from './clock_panel.style'

export function ClockPanel() {
  const actions = useNewTabActions()

  const showClock = useNewTabState((s) => s.showClock)
  const clockFormat = useNewTabState((s) => s.clockFormat)

  function formatOptionText(format: ClockFormat) {
    switch (format) {
      case ClockFormat.k12:
        return getString(S.NEW_TAB_CLOCK_FORMAT_OPTION12HOUR_TEXT)
      case ClockFormat.k24:
        return getString(S.NEW_TAB_CLOCK_FORMAT_OPTION24HOUR_TEXT)
      default:
        return formatString(
          getString(S.NEW_TAB_CLOCK_FORMAT_OPTION_AUTOMATIC_TEXT),
          [new Intl.DateTimeFormat(undefined).resolvedOptions().locale],
        )
    }
  }

  function renderFormatOption(format: ClockFormat) {
    return (
      <leo-option value={String(format)}>{formatOptionText(format)}</leo-option>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString(S.NEW_TAB_SHOW_CLOCK_LABEL)}</label>
        <Toggle
          size='small'
          checked={showClock}
          onChange={({ checked }) => {
            actions.setShowClock(checked)
          }}
        />
      </div>
      <div className='control-row'>
        <label>{getString(S.NEW_TAB_CLOCK_FORMAT_LABEL)}</label>
        <DropDown
          value={String(clockFormat)}
          positionStrategy='fixed'
          onChange={(detail) => {
            actions.setClockFormat(Number(detail.value) || (0 as ClockFormat))
          }}
        >
          <span slot='value'>{formatOptionText(clockFormat)}</span>
          {renderFormatOption(ClockFormat.kAuto)}
          {renderFormatOption(ClockFormat.k12)}
          {renderFormatOption(ClockFormat.k24)}
        </DropDown>
      </div>
    </div>
  )
}
