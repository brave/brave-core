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

class BraveShields extends Component {
  render () {
    const { shieldsPanel, actions } = this.props
    return <div data-test-id='brave-shields-panel'>
      <BraveShieldsHeader
        hostname={shieldsPanel.hostname}
        shieldsPanel={shieldsPanel.braveShields}
        />
      <BraveShieldsStats />
      <BraveShieldsControls
        adBlock={shieldsPanel.adBlock}
        trackingProtection={shieldsPanel.trackingProtection}
        toggleAdBlock={actions.toggleAdBlock}
        toggleTrackingProtection={actions.toggleTrackingProtection}
        />
      <BraveShieldsFooter />
    </div>
  }
}

export default connect(
  state => ({
    shieldsPanel: state.shieldsPanel
  }),
  dispatch => ({
    actions: bindActionCreators(shieldsPanelActions, dispatch)
  })
)(BraveShields)
