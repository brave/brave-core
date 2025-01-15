/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../locale_context'
import { useNewTabModel, useNewTabState } from '../new_tab_context'

import { style } from './stats_panel.style'

export function StatsPanel() {
  const { getString } = useLocale()
  const model = useNewTabModel()

  const showStats = useNewTabState((state) => state.showShieldsStats)

  return (
    <div {...style}>
      <div className='form-control-row'>
        <label>{getString('showStatsLabel')}</label>
        <Toggle
          size='small'
          checked={showStats}
          onChange={({ checked }) => { model.setShowShieldsStats(checked) }}
        />
      </div>
    </div>
  )
}
