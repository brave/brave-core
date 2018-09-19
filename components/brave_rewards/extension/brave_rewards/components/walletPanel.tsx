/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Constants
import { ApplicationState, ComponentProps } from '../constants/rewardsPanelState'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

interface Props extends ComponentProps {
}

export class WalletPanel extends React.Component<Props, {}> {
  render () {
    return (
      <>
        This is wallet panel content
      </>
    )
  }
}

export const mapStateToProps = (state: ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(WalletPanel)
