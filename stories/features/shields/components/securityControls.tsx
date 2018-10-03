/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Stat, GridLabel } from '../../../../src/features/shields'
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
}

export default class SecurityControls extends React.PureComponent<Props, {}> {
  render () {
    const { enabled } = this.props
    if (!enabled) {
      return null
    }
    return (
      <>
        {/* pishing toggle */}
        <GridLabel>
          <Stat>{data.pishingMalwareBlocked}</Stat>
          {locale.blockPishing}
        </GridLabel>
        {/* connections encrypted toggle */}
        <GridLabel>
          <Stat>{data.connectionsEncrypted}</Stat>
          {locale.connectionsEncrypted}
        </GridLabel>
      </>
    )
  }
}
