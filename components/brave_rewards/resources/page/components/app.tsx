/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import SettingsPage from './settingsPage'

// Utils
import * as rewardsActions from '../actions/rewards_actions'

interface Props extends Rewards.ComponentProps {
}

export class App extends React.Component<Props> {
  componentDidMount () {
    this.actions.isInitialized()

    if (!this.props.rewardsData.enabledAdsMigrated) {
      const { adsEnabled, adsIsSupported } = this.props.rewardsData.adsData

      if (adsIsSupported) {
        this.props.actions.onAdsSettingSave('adsEnabledMigrated', adsEnabled)
      }
    }
    this.props.actions.onlyAnonWallet()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    return (
      <div id='rewardsPage'>
        <SettingsPage />
      </div>
    )
  }
}

export const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(App)
