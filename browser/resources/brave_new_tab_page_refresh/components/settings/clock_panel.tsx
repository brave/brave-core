/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Toggle from '@brave/leo/react/toggle'

import { ClockFormat } from '../../models/new_tab'
import { useLocale } from '../context/locale_context'
import { useAppActions, useAppState } from '../context/app_model_context'
import formatMessage from '$web-common/formatMessage'

import { style } from './clock_panel.style'

export function ClockPanel() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const [showClock, clockFormat] = useAppState((state) => [
    state.showClock,
    state.clockFormat
  ])

  function formatOptionText(format: ClockFormat) {
    switch (format) {
      case ClockFormat.k12:
        return getString('clockFormatOption12HourText')
      case ClockFormat.k24:
        return getString('clockFormatOption24HourText')
      default:
        return formatMessage(getString('clockFormatOptionAutomaticText'), [
          new Intl.DateTimeFormat(undefined).resolvedOptions().locale
        ]).join('')
    }
  }

  function renderFormatOption(format: ClockFormat) {
    return (
      <leo-option value={String(format)}>{formatOptionText(format)}</leo-option>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='form-control-row'>
        <label>{getString('showClockLabel')}</label>
        <Toggle
          size='small'
          checked={showClock}
          onChange={({ checked }) => { actions.setShowClock(checked) }}
        />
      </div>
      <div>
        <label>{getString('clockFormatLabel')}</label>
        <DropDown
          value={String(clockFormat)}
          positionStrategy='fixed'
          onChange={(detail) => {
            actions.setClockFormat(Number(detail.value) || 0 as ClockFormat)
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
