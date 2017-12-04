/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import * as shieldsPanelActions from '../actions/shieldsPanelActions'
import BraveShieldsHeader from '../components/braveShields/braveShieldsHeader'
import BraveShieldsStats from '../components/braveShields/braveShieldsStats'
import BraveShieldsControls from '../components/braveShields/braveShieldsControls'
import BraveShieldsFooter from '../components/braveShields/braveShieldsFooter'
import * as shieldsPanelState from '../state/shieldsPanelState'

class BraveShields extends Component {
  render () {
    const { shieldsPanelTabData, actions } = this.props
    if (!shieldsPanelTabData) {
      return null
    }
    return <div data-test-id='brave-shields-panel'>
      <BraveShieldsHeader
        shieldsEnabled={shieldsPanelTabData.shieldsEnabled}
        shieldsToggled={actions.shieldsToggled}
        hostname={shieldsPanelTabData.hostname}
        />
      <BraveShieldsStats
        shieldsEnabled={shieldsPanelTabData.shieldsEnabled}
        adsBlocked={shieldsPanelTabData.adsBlocked}
        trackingProtectionBlocked={shieldsPanelTabData.trackingProtectionBlocked}
        httpsEverywhereRedirected={shieldsPanelTabData.httpsEverywhereRedirected}
        javascriptBlocked={shieldsPanelTabData.javascriptBlocked}
      />
      <BraveShieldsControls
        shieldsEnabled={shieldsPanelTabData.shieldsEnabled}
        blockAdsTrackers={actions.blockAdsTrackers}
        adsTrackers={shieldsPanelTabData.adsTrackers}
        httpsEverywhere={shieldsPanelTabData.httpsEverywhere}
        javascript={shieldsPanelTabData.javascript}
        controlsToggled={actions.controlsToggled}
        httpsEverywhereToggled={actions.httpsEverywhereToggled}
        javascriptToggled={actions.javascriptToggled}
        controlsOpen={shieldsPanelTabData.controlsOpen}
      />
      <BraveShieldsFooter />
    </div>
  }
}

export default connect(
  (state) => ({
    shieldsPanelTabData: shieldsPanelState.getActiveTabData(state.shieldsPanel)
  }),
  (dispatch) => ({
    actions: bindActionCreators(shieldsPanelActions, dispatch)
  })
)(BraveShields)
