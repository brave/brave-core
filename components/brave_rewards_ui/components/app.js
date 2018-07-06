/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const rewardsActions = require('../actions/rewards_actions')

const CreateWalletLink = (props) =>
  <div>
    <a href='#' onClick={props.createWalletClicked}>Create Wallet</a>
  </div>

class RewardsPage extends React.Component {
  constructor (props) {
    super(props)
    this.onCreateWalletClicked = this.onCreateWalletClicked.bind(this)
  }

  onCreateWalletClicked() {
    this.actions.createWalletRequested()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { rewardsData } = this.props
    return (
      <div>
        <CreateWalletLink createWalletClicked={this.onCreateWalletClicked} />
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
      </div>)
  }
}

const mapStateToProps = (state) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPage)
