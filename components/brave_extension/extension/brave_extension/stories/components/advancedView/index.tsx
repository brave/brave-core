/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ShieldsPanel } from '../../../components'

// Components group
import Header from './header'
import InterfaceControls from './interfaceControls'
import PrivacyControls from './privacyControls'
import Footer from '../shared/footer'
import WebCompatWarning from './overlays/webCompatWarningOverlay'

interface Props {
  enabled: boolean
  firstAccess: boolean
  hostname: string
  advancedView: boolean
  favicon: string
  adsTrackersBlocked: number
  httpsUpgrades: number
  scriptsBlocked: number
  fingerprintingBlocked: number
  fakeOnChangeShieldsEnabled: () => void
  fakeOnChangeAdvancedView: () => void
  fakeToggleFirstAccess: () => void
}

interface State {
  isBlockedListOpen: boolean
}

export default class ShieldsAdvancedView extends React.PureComponent<Props, State> {
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
      firstAccess,
      favicon,
      hostname,
      adsTrackersBlocked,
      httpsUpgrades,
      scriptsBlocked,
      fingerprintingBlocked,
      fakeOnChangeShieldsEnabled,
      fakeOnChangeAdvancedView,
      fakeToggleFirstAccess
    } = this.props
    const { isBlockedListOpen } = this.state
    return (
      <ShieldsPanel style={{ width: '370px' }}>
        {firstAccess ? <WebCompatWarning onConfirm={fakeToggleFirstAccess} /> : null}
        <Header
          enabled={enabled}
          favicon={favicon}
          hostname={hostname}
          isBlockedListOpen={isBlockedListOpen}
          adsTrackersBlocked={adsTrackersBlocked}
          httpsUpgrades={httpsUpgrades}
          scriptsBlocked={scriptsBlocked}
          fingerprintingBlocked={fingerprintingBlocked}
          fakeOnChangeShieldsEnabled={fakeOnChangeShieldsEnabled}
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
        <Footer
          advancedView={true}
          isBlockedListOpen={isBlockedListOpen}
          fakeOnChangeAdvancedView={fakeOnChangeAdvancedView}
        />
      </ShieldsPanel>
    )
  }
}
