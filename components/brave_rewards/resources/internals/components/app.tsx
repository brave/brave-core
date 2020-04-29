/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Contributions } from './contributions'
import { WalletInfo } from './walletInfo'
import { Balance } from './balance'
import { Promotions } from './promotions'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsInternalsActions from '../actions/rewards_internals_actions'

interface Props {
  actions: any
  rewardsInternalsData: RewardsInternals.State
}

export class RewardsInternalsPage extends React.Component<Props, {}> {
  componentDidMount () {
    this.getData()
  }

  get actions () {
    return this.props.actions
  }

  getData = () => {
    this.actions.getRewardsEnabled()
    this.actions.getRewardsInternalsInfo()
    this.actions.getBalance()
    this.actions.getPromotions()
  }

  onRefresh = () => {
    this.getData()
  }

  render () {
    const { isRewardsEnabled, balance, info, promotions } = this.props.rewardsInternalsData
    if (isRewardsEnabled) {
      return (
        <div id='rewardsInternalsPage'>
          <WalletInfo state={this.props.rewardsInternalsData} />
          <Balance info={balance} />
          <Promotions items={promotions} />
          <br/>
          <br/>
          <button type='button' onClick={this.onRefresh}>{getLocale('refreshButton')}</button>
          <Contributions items={info.currentReconciles} />
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
