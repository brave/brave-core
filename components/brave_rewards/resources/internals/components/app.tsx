/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { CurrentReconcile } from './currentReconcile'
import { KeyInfoSeed } from './keyInfoSeed'
import { WalletPaymentId } from './walletPaymentId'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsInternalsActions from '../actions/rewards_internals_actions'

interface Props {
  actions: any
  rewardsInternalsData: RewardsInternals.State
}

export class RewardsInternalsPage extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  render () {
    const { rewardsInternalsData } = this.props
    if (rewardsInternalsData.info.isRewardsEnabled) {
      return (
        <div id='rewardsInternalsPage'>
          <KeyInfoSeed isKeyInfoSeedValid={rewardsInternalsData.info.isKeyInfoSeedValid || ''} />
          <WalletPaymentId walletPaymentId={rewardsInternalsData.info.walletPaymentId || ''} />
          <hr/>
          {rewardsInternalsData.info.currentReconciles.map((item, index) => (
            <div>
              <span i18n-content='currentReconcile'/> {index + 1}
              <CurrentReconcile currentReconcile={item || ''} />
            </div>
          ))}
        </div>)
    } else {
      return (
        <div id='rewardsInternalsPage'>
          {getLocale('rewardsNotEnabled')} <a href='chrome://rewards' target='_blank'>chrome://rewards</a>
        </div>)
    }
  }
}

export const mapStateToProps = (state: RewardsInternals.ApplicationState) => ({
  rewardsInternalsData: state.rewardsInternalsData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsInternalsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsInternalsPage)
