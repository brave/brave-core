/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ShieldsPanel } from '../../../../../src/features/shields'

// Components group
import Header from './header'
import Footer from '../shared/footer'

interface Props {
  enabled: boolean
  hostname: string
  advancedView: boolean
  favicon: string
  adsTrackersBlocked: number
  httpsUpgrades: number
  scriptsBlocked: number
  fingerprintingBlocked: number
  fakeOnChangeShieldsEnabled: () => void
  fakeOnChangeAdvancedView: () => void
  fakeOnChangeReadOnlyView: () => void
}

interface State {
  isBlockedListOpen: boolean
}

export default class ShieldsSimpleView extends React.PureComponent<Props, State> {
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
      fakeOnChangeShieldsEnabled,
      fakeOnChangeReadOnlyView,
      fakeOnChangeAdvancedView
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
          fakeOnChangeShieldsEnabled={fakeOnChangeShieldsEnabled}
          fakeOnChangeReadOnlyView={fakeOnChangeReadOnlyView}
        />
        <Footer
          advancedView={false}
          isBlockedListOpen={isBlockedListOpen}
          fakeOnChangeAdvancedView={fakeOnChangeAdvancedView}
        />
      </ShieldsPanel>
    )
  }
}
