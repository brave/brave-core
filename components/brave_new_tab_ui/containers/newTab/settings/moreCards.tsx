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
  toggleShowBinance: () => void
  showBinance: boolean
  binanceSupported: boolean
}

class MoreCardsSettings extends React.PureComponent<Props, {}> {
  render () {
    const { binanceSupported, toggleShowBinance, showBinance } = this.props
    return (
      <div>
        {
          binanceSupported
          ? <SettingsRow>
              <SettingsText>{getLocale('showBinance')}</SettingsText>
              <Toggle
                onChange={toggleShowBinance}
                checked={showBinance}
                size='large'
              />
            </SettingsRow>
          : null
        }
      </div>
    )
  }
}

export default MoreCardsSettings
