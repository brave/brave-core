/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group components
import AdsTrackersControl from './controls/adsTrackersControl'
import HTTPSUpgradesControl from './controls/httpsUpgradesControl'

export default class InterfaceControls extends React.PureComponent<{}, {}> {
  render () {
    return (
      <>
        <AdsTrackersControl />
        <HTTPSUpgradesControl />
      </>
    )
  }
}
