/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group Components
import AdsTrackersControl from './controls/adsTrackersControl'
import HTTPSUpgradesControl from './controls/httpsUpgradesControl'

interface Props {
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  adsTrackersBlocked: number
  httpsUpgrades: number
}

interface State {
  connectionsUpgradedOpen: boolean
  connectionsUpgradedEnabled: boolean
}

export default class InterfaceControls extends React.PureComponent<Props, State> {
  render () {
    const {
      favicon,
      hostname,
      isBlockedListOpen,
      setBlockedListOpen,
      adsTrackersBlocked,
      httpsUpgrades
    } = this.props
    return (
      <>
        <AdsTrackersControl
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          adsTrackersBlocked={adsTrackersBlocked}
          setBlockedListOpen={setBlockedListOpen}
        />
        <HTTPSUpgradesControl
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          httpsUpgrades={httpsUpgrades}
          setBlockedListOpen={setBlockedListOpen}
        />
      </>
    )
  }
}
