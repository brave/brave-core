/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group Components
import ScriptsControl from './controls/scriptsControl'
import CookiesControl from './controls/cookiesControl'
import DeviceRecognitionControl from './controls/deviceRecognitionControl'

interface Props {
  favicon: string
  hostname: string
  setBlockedListOpen: () => void
  isBlockedListOpen: boolean
  scriptsBlocked: number
  fingerprintingBlocked: number
}

interface State {
  deviceRecognitionOpen: boolean
  scriptsBlockedOpen: boolean
  scriptsBlockedEnabled: boolean
}

export default class PrivacyControls extends React.PureComponent<Props, State> {
  render () {
    const {
      favicon,
      hostname,
      setBlockedListOpen,
      isBlockedListOpen,
      scriptsBlocked,
      fingerprintingBlocked
    } = this.props
    return (
      <>
        <ScriptsControl
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          scriptsBlocked={scriptsBlocked}
          setBlockedListOpen={setBlockedListOpen}
        />
        <CookiesControl
          isBlockedListOpen={isBlockedListOpen}
        />
        <DeviceRecognitionControl
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          fingerprintingBlocked={fingerprintingBlocked}
          setBlockedListOpen={setBlockedListOpen}
        />
      </>
    )
  }
}
