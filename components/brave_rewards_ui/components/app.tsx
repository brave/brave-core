/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const rewardsActions = require('../actions/rewards_actions')

class RewardsPage extends React.Component {
  onCreateWalletClicked = () => {
    this.actions.createWalletRequested()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { rewardsData } = this.props
    return (
      <div>
        <div>
          <a href='#' onClick={this.onCreateWalletClicked}>Create Wallet</a>
        </div>
        {
          rewardsData.walletCreated
          ? <div>Wallet Created!</div>
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

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: any) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPage)
