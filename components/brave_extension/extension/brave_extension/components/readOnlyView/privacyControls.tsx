/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group components
import ScriptsControl from './controls/scriptsControl'
import CookiesControl from './controls/cookiesControl'
import DeviceRecognitionControl from './controls/deviceRecognitionControl'

export default class PrivacyControls extends React.PureComponent<{}, {}> {
  render () {
    return (
      <>
        <ScriptsControl />
        <CookiesControl />
        <DeviceRecognitionControl />
      </>
    )
  }
}
