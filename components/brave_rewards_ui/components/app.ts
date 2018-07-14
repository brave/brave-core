/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const rewardsActions = require('../actions/rewards_actions')

const CreateWalletLink = (props) =>
  href = '#' onClick = { props.createWalletClicked } > Create Wallet< /a>
  < /div>

class RewardsPage extends React.Component {
  constructor (props) {
    super(props)
    this.onCreateWalletClicked = this.onCreateWalletClicked.bind(this)
  }

  onCreateWalletClicked () {
    this.actions.createWalletRequested()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { rewardsData } = this.props
    return (
      createWalletClicked = { this.onCreateWalletClicked } / >
          {
            rewardsData.walletCreated
          ? Wallet  Created!/div> as 
          : null
          }
    {
          rewardsData.walletCreateFailed
          ? Wallet  Create Failed! < /div>
          : null
        }
    /div>) as 
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
