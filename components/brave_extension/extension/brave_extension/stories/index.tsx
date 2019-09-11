/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components group
import SimpleView from './components/simpleView'
import AdvancedView from './components/advancedView'

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
  fakeOnChangeReadOnlyView: () => void
  fakeToggleFirstAccess: () => void
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
      firstAccess,
      hostname,
      favicon,
      adsTrackersBlocked,
      httpsUpgrades,
      scriptsBlocked,
      fingerprintingBlocked,
      fakeOnChangeShieldsEnabled,
      fakeOnChangeAdvancedView,
      fakeOnChangeReadOnlyView,
      fakeToggleFirstAccess
    } = this.props
    const { advancedView } = this.props
    return advancedView
      ? (
        <AdvancedView
          enabled={enabled}
          firstAccess={firstAccess}
          hostname={hostname}
          advancedView={advancedView}
          favicon={favicon}
          adsTrackersBlocked={adsTrackersBlocked}
          httpsUpgrades={httpsUpgrades}
          scriptsBlocked={scriptsBlocked}
          fingerprintingBlocked={fingerprintingBlocked}
          fakeOnChangeShieldsEnabled={fakeOnChangeShieldsEnabled}
          fakeOnChangeAdvancedView={fakeOnChangeAdvancedView}
          fakeToggleFirstAccess={fakeToggleFirstAccess}
        />
      ) : (
        <SimpleView
          enabled={enabled}
          hostname={hostname}
          advancedView={advancedView}
          favicon={favicon}
          adsTrackersBlocked={adsTrackersBlocked}
          httpsUpgrades={httpsUpgrades}
          scriptsBlocked={scriptsBlocked}
          fingerprintingBlocked={fingerprintingBlocked}
          fakeOnChangeShieldsEnabled={fakeOnChangeShieldsEnabled}
          fakeOnChangeAdvancedView={fakeOnChangeAdvancedView}
          fakeOnChangeReadOnlyView={fakeOnChangeReadOnlyView}
        />
      )
  }
}
