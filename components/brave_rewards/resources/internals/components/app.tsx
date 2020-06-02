/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Contributions } from './contributions'
import { Promotions } from './promotions'
import { General } from './general'
import { Log } from './log'
import { Tabs } from 'brave-ui/components'
import { Wrapper, MainTitle, DisabledContent } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsInternalsActions from '../actions/rewards_internals_actions'

interface Props {
  actions: any
  rewardsInternalsData: RewardsInternals.State
}

interface State {
  currentTabId: string
}

export class RewardsInternalsPage extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentTabId: 'generalInfo'
    }
    this.getLog = this.getLog.bind(this)
    this.clearLog = this.clearLog.bind(this)
    this.getGeneralInfo = this.getGeneralInfo.bind(this)
    this.getContributions = this.getContributions.bind(this)
  }

  componentDidMount () {
    this.getGeneralInfo()
  }

  get actions () {
    return this.props.actions
  }

  getGeneralInfo = () => {
    this.actions.getRewardsEnabled()
    this.actions.getRewardsInternalsInfo()
    this.actions.getBalance()
  }

  onTabChange = (tabId: string) => {
    this.setState({ currentTabId: tabId })

    switch (tabId) {
      case 'promotions': {
        this.getPromotions()
        break
      }
      case 'contributions': {
        this.getContributions()
        break
      }
      case 'generalInfo': {
        this.getGeneralInfo()
        break
      }
    }
  }

  clearLog = () => {
    this.actions.clearLog()
  }

  getLog = () => {
    this.actions.getLog()
  }

  getPromotions = () => {
    this.actions.getPromotions()
  }

  getContributions = () => {
    // TODO add
  }

  render () {
    const { isRewardsEnabled, info, promotions, log } = this.props.rewardsInternalsData

    if (!isRewardsEnabled) {
      return (
        <Wrapper id='rewardsInternalsPage'>
          <MainTitle level={2}>{getLocale('mainTitle')}</MainTitle>
          <DisabledContent>
            {getLocale('rewardsNotEnabled')} <a href='brave://rewards' target='_blank'>brave://rewards</a>
          </DisabledContent>
        </Wrapper>)
    }

    return (
      <Wrapper id='rewardsInternalsPage'>
        <MainTitle level={2}>{getLocale('mainTitle')}</MainTitle>
        <Tabs
          activeTabId={this.state.currentTabId}
          onChange={this.onTabChange}
        >
          <div data-key='generalInfo' data-title={getLocale('tabGeneralInfo')}>
            <General data={this.props.rewardsInternalsData} onGet={this.getGeneralInfo} />
          </div>
          <div data-key='logs' data-title={getLocale('tabLogs')}>
            <Log log={log} onGet={this.getLog} onClear={this.clearLog} />
          </div>
          <div data-key='promotions' data-title={getLocale('tabPromotions')}>
            <Promotions items={promotions} onGet={this.getPromotions} />
          </div>
          <div data-key='contributions' data-title={getLocale('tabContributions')}>
            <Contributions items={info.currentReconciles} onGet={this.getContributions} />
          </div>
        </Tabs>
      </Wrapper>)
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
