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
  onlyAnonWallet: boolean
}

export class RewardsPanel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      tabId: -1,
      onlyAnonWallet: false
    }
  }

  componentDidMount () {
    chrome.braveRewards.isInitialized((initialized: boolean) => {
      if (initialized) {
        this.props.actions.initialized()
        this.startRewards()
      }
    })

    chrome.braveRewards.onlyAnonWallet((only: boolean) => {
      this.setState({
        onlyAnonWallet: !!only
      })
    })
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (prevProps.rewardsPanelData.initializing &&
        !this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }
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
      const pollTwitchPage = (tab: chrome.tabs.Tab, tabId: number) => {
        // use an interval here to monitor when the DOM has finished
        // generating. clear after the data is present.
        // Check every second no more than 'limit' times
        // clear the interval if panel closes

        const markupMatch = 'channel-header'
        let itr = 0
        const limit = 5
        let interval = setInterval(poll, 1000)
        function poll () {
          chrome.tabs.executeScript(tabId, {
            code: 'document.body.outerHTML'
          }, function (result: string[]) {
            if (result[0].includes(markupMatch)) {
              clearInterval(interval)
              const rewardsPanelActions = require('../background/actions/rewardsPanelActions').default
              rewardsPanelActions.onTabRetrieved(tab, false, result[0])
            } else {
              chrome.storage.local.get(['rewards_panel_open'], function (result) {
                if (result['rewards_panel_open'] === 'false') {
                  // panel was closed. give up
                  clearInterval(interval)
                }
              })
              itr++
              if (itr === limit) {
                // give up
                clearInterval(interval)

                getTabData(tabId)
              }
            }
          })
        }
        poll()
      }

      const pollData = (tab: chrome.tabs.Tab, tabId: number, url: URL) => {
        if (url && url.href.startsWith('https://www.twitch.tv/')) {
          chrome.storage.local.get(['rewards_panel_open'], function (result) {
            if (result['rewards_panel_open'] === 'true') {
              pollTwitchPage(tab, tabId)
            }
          })
        } else {
          getTabData(tabId)
        }
      }
      let tab = tabs[0]
      if (tab.url && tab.id) {
        let url = new URL(tab.url)
        if (url && url.host.endsWith('.twitch.tv')) {
          pollData(tab, tab.id, url)
        } else {
          getTabData(tab.id)
        }
      } else if (tab.id) {
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
    return (
      <Panel
        tabId={this.state.tabId}
        onlyAnonWallet={this.state.onlyAnonWallet}
      />
    )
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
