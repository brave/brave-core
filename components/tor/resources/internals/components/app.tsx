/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { GeneralInfo } from './generalInfo'
import { Log } from './log'
import { TorControlEvents } from './torControlEvents'
import { Tabs } from 'brave-ui/components'

// Utils
import { getLocale } from '../../../../common/locale'
import * as torInternalsActions from '../actions/tor_internals_actions'

interface Props {
  actions: any
  torInternalsData: TorInternals.State
}

interface State {
  currentTabId: string
}

export class TorInternalsPage extends React.Component<Props, State> {
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
    this.actions.getTorGeneralInfo()
  }

  getLog = () => {
    this.actions.getTorLog()
  }

  onTabChange = (tabId: string) => {
    this.setState({ currentTabId: tabId })

    switch (tabId) {
      case 'generalInfo': {
        this.getGeneralInfo()
        break
      }
      case 'log': {
        this.getLog()
        break
      }
      default:
        break
    }
  }

  render () {
    return (
        <Tabs
          id={'internals-tabs'}
          activeTabId={this.state.currentTabId}
          onChange={this.onTabChange}
        >
          <div data-key='generalInfo' data-title={getLocale('tabGeneralInfo')}>
            <GeneralInfo state={this.props.torInternalsData} />
          </div>
          <div data-key='log' data-title={getLocale('tabLogs')}>
	   <Log log={this.props.torInternalsData.log}/>
          </div>
          <div data-key='torControlEvents' data-title={getLocale('torControlEvents')}>
	   <TorControlEvents events={this.props.torInternalsData.torControlEvents}/>
          </div>
        </Tabs>
    )
  }
}

export const mapStateToProps = (state: TorInternals.ApplicationState) => ({
  torInternalsData: state.torInternalsData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(torInternalsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(TorInternalsPage)
