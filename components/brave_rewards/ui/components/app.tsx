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
import { WelcomePage } from 'brave-ui/features/rewards'

interface Props extends Rewards.ComponentProps {
}

export class App extends React.Component<Props, {}> {
  componentDidMount () {
    if (!this.props.rewardsData.walletCreated) {
      this.actions.checkWalletExistence()
    }
  }

  onCreateWalletClicked = () => {
    this.actions.createWallet()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { rewardsData } = this.props

    return (
      <div id='rewardsPage'>
        {
          !rewardsData.walletCreated && !rewardsData.walletCreateFailed
          ? <WelcomePage
            optInAction={this.onCreateWalletClicked}
          />
          : null
        }
        {
          rewardsData.walletCreated
          ? <SettingsPage />
          : null
        }
        {
          rewardsData.walletCreateFailed
          ? <div>Wallet Create Failed!</div>
          : null
        }
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
