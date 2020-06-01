// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import { Toggle } from '../../../components/toggle'

import { getLocale } from '../../../../common/locale'

interface Props {
  toggleShowStats: () => void
  showStats: boolean
}

class BraveStatsSettings extends React.PureComponent<Props, {}> {
  render () {
    const { toggleShowStats, showStats } = this.props

    return (
      <div>
        <SettingsRow>
          <SettingsText>{getLocale('showBraveStats')}</SettingsText>
          <Toggle
            onChange={toggleShowStats}
            checked={showStats}
            size='large'
          />
        </SettingsRow>
      </div>
    )
  }
}

export default BraveStatsSettings
