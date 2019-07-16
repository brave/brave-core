/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import * as shieldsPanelActions from '../actions/shieldsPanelActions'
import * as shieldsPanelState from '../state/shieldsPanelState'
import BraveShields from '../containers/braveShields'
import { State } from '../types/state/mainState'

const mapStateToProps = (
  state: State,
  ownProps: {
    settings: chrome.braveShields.BraveShieldsViewPreferences
  }
) => {
  return ({
    shieldsPanelTabData: shieldsPanelState.getActiveTabData(state.shieldsPanel),
    persistentData: shieldsPanelState.getPersistentData(state.shieldsPanel),
    settings: ownProps.settings
  })
}

const mapDispatchToProps = (dispatch: any) => ({
  actions: bindActionCreators(shieldsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(BraveShields as any) // TODO remove any
