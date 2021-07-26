/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { AdDiagnostics } from './ad_diagnostics'
import { Contributions } from './contributions'
import { Promotions } from './promotions'
import { General } from './general'
import { EventLogs } from './event_logs'
import { Log } from './log'
import { Tabs } from 'brave-ui/components'
import { Wrapper, MainTitle, Disclaimer } from '../style'

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
  }

  componentDidMount () {
    this.getGeneralInfo()
  }

  get actions () {
    return this.props.actions
  }

  getGeneralInfo = () => {
    this.actions.getRewardsInternalsInfo()
    this.actions.getBalance()
    this.actions.getExternalWallet()
  }

  onTabChange = (tabId: string) => {
    this.setState({ currentTabId: tabId })

    switch (tabId) {
      case 'generalInfo': {
        this.getGeneralInfo()
        break
      }
      case 'promotions': {
        this.getPromotions()
        break
      }
      case 'contributions': {
        this.getContributions()
        break
      }
      case 'eventLogs': {
        this.getEventLogs()
        break
      }
      case 'adDiagnostics': {
        this.getAdDiagnostics()
        break
      }
    }
  }

  clearLog = () => {
    this.actions.clearLog()
  }

  getPartialLog = () => {
    this.actions.getPartialLog()
  }

  getFullLog = () => {
    this.actions.getFullLog()
  }

  downloadCompleted = () => {
    this.actions.downloadCompleted()
  }

  getPromotions = () => {
    this.actions.getPromotions()
  }

  getContributions = () => {
    this.actions.getContributions()
  }

  getEventLogs = () => {
    this.actions.getEventLogs()
  }

  getAdDiagnostics = () => {
    this.actions.getAdDiagnostics()
  }

  render () {
    const {
      contributions,
      promotions,
      log,
      fullLog,
      eventLogs,
      adDiagnostics
    } = this.props.rewardsInternalsData

    return (
      <Wrapper id='rewardsInternalsPage'>
        <MainTitle level={2}>{getLocale('mainTitle')}</MainTitle>
        <Disclaimer>{getLocale('mainDisclaimer')}</Disclaimer>
        <Tabs
          id={'internals-tabs'}
          activeTabId={this.state.currentTabId}
          onChange={this.onTabChange}
        >
          <div data-key='generalInfo' data-title={getLocale('tabGeneralInfo')}>
            <General data={this.props.rewardsInternalsData} onGet={this.getGeneralInfo} />
          </div>
          <div data-key='logs' data-title={getLocale('tabLogs')}>
            <Log
              log={log}
              fullLog={fullLog}
              onGet={this.getPartialLog}
              onFullLog={this.getFullLog}
              onClear={this.clearLog}
              onDownloadCompleted={this.downloadCompleted}
            />
          </div>
          <div data-key='promotions' data-title={getLocale('tabPromotions')}>
            <Promotions items={promotions} onGet={this.getPromotions} />
          </div>
          <div data-key='contributions' data-title={getLocale('tabContributions')}>
            <Contributions items={contributions} onGet={this.getContributions} />
          </div>
          <div data-key='eventLogs' data-title={getLocale('tabEventLogs')}>
            <EventLogs items={eventLogs} />
          </div>
          <div data-key='adDiagnostics' data-title={getLocale('tabAdDiagnostics')}>
            <AdDiagnostics entries={adDiagnostics} onGet={this.getAdDiagnostics}/>
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
