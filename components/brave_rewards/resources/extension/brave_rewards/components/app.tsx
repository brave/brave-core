/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { getTabData } from '../background/api/tabs_api'

// Components
import Panel from './panel'

// Utils
import * as utils from '../utils'
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

interface Props extends RewardsExtension.ComponentProps {
}

interface State {
  tabId: number
}

export class RewardsPanel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      tabId: -1
    }
  }

  componentDidMount () {
    chrome.braveRewards.isInitialized((initialized: boolean) => {
      if (initialized) {
        this.props.actions.initialized()
        this.startRewards()
      }
    })
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (prevProps.rewardsPanelData.initializing &&
        !this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }
  }

  getPublisherPanelInfoForCurrentTab () {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }

      const tab = tabs[0]
      if (!tab || !tab.id || !tab.url || tab.incognito || !tab.active) {
        return
      }

      const braveExtensionId = 'mnojpmjdmbbfmejpflffifhffcmidifd'

      chrome.runtime.sendMessage(
        braveExtensionId,
        {
          type: 'GetPublisherPanelInfo',
          tabId: tab.id
        })
    })
  }

  startRewards = () => {
    this.getCurrentTab(this.onCurrentTab)

    chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
      this.props.actions.onAllNotifications(list)
    })

    const { externalWallet } = this.props.rewardsPanelData

    utils.getExternalWallet(this.actions, externalWallet)

    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      this.actions.onBalance(balance)
    })

    chrome.braveRewards.getRewardsParameters((parameters: RewardsExtension.RewardsParameters) => {
      rewardsPanelActions.onRewardsParameters(parameters)
    })

    chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
      this.props.actions.onAllNotifications(list)
    })

    this.getCurrentTab(this.onCurrentTab)

    this.handleGrantNotification()

    this.getPublisherPanelInfoForCurrentTab()
  }

  handleGrantNotification = () => {
    const hash = window && window.location && window.location.hash

    if (!hash) {
      return
    }

    if (!hash.startsWith('#grant_')) {
      return
    }

    let promotionId = hash.split('#grant_')[1]

    if (!promotionId) {
      return
    }

    chrome.braveRewards.claimPromotion(promotionId, (properties: RewardsExtension.Captcha) => {
      this.actions.onClaimPromotion(properties)
    })
  }

  getCurrentTab (callback: ((tab: chrome.tabs.Tab) => void)) {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      callback(tabs[0])
    })
  }

  getTabData () {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      let tab = tabs[0]
      if (tab.id) {
        getTabData(tab.id)
      }
    })
  }

  onCurrentTab = (tab: chrome.tabs.Tab) => {
    if (!tab || !tab.id) {
      return
    }

    this.setState({
      tabId: tab.id
    })

    this.getTabData()
  }

  get actions () {
    return this.props.actions
  }

  render () {
    return <Panel tabId={this.state.tabId} />
  }
}

export const mapStateToProps = (state: RewardsExtension.ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPanel)
