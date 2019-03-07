/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ShieldsPanel } from '../../../src/features/shields'

// Components group
import Header from './components/header'
import InterfaceControls from './components/interfaceControls'
import PrivacyControls from './components/privacyControls'
import Footer from './components/footer'

interface Props {
  enabled: boolean
  hostname: string
  favicon: string
  adsTrackersBlocked: number
  httpsUpgrades: number
  scriptsBlocked: number
  fingerprintingBlocked: number
  fakeOnChange: () => void
}

interface State {
  isBlockedListOpen: boolean
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { isBlockedListOpen: false }
  }
  setBlockedListOpen = () => {
    this.setState({ isBlockedListOpen: !this.state.isBlockedListOpen })
  }
  render () {
    const {
      enabled,
      favicon,
      hostname,
      adsTrackersBlocked,
      httpsUpgrades,
      scriptsBlocked,
      fingerprintingBlocked,
      fakeOnChange
    } = this.props
    const { isBlockedListOpen } = this.state
    return (
      <ShieldsPanel style={{ width: '370px' }}>
        <Header
          enabled={enabled}
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          adsTrackersBlocked={adsTrackersBlocked}
          httpsUpgrades={httpsUpgrades}
          scriptsBlocked={scriptsBlocked}
          fingerprintingBlocked={fingerprintingBlocked}
          fakeOnChange={fakeOnChange}
        />
        {
          enabled ? (
            <>
              <InterfaceControls
                favicon={favicon}
                hostname={hostname}
                setBlockedListOpen={this.setBlockedListOpen}
                isBlockedListOpen={isBlockedListOpen}
                adsTrackersBlocked={adsTrackersBlocked}
                httpsUpgrades={httpsUpgrades}
              />
              <PrivacyControls
                favicon={favicon}
                hostname={hostname}
                setBlockedListOpen={this.setBlockedListOpen}
                isBlockedListOpen={isBlockedListOpen}
                scriptsBlocked={scriptsBlocked}
                fingerprintingBlocked={fingerprintingBlocked}
              />
            </>
          ) : null
        }
        <Footer isBlockedListOpen={isBlockedListOpen} />
      </ShieldsPanel>
    )
  }
}
