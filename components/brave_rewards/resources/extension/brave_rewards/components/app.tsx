/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { DisabledPanel, PanelWelcome } from 'brave-ui/features/rewards'

// Components
import Panel from './panel'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

interface Props extends RewardsExtension.ComponentProps {
}

interface State {
  windowId: number
}

export class RewardsPanel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      windowId: -1
    }
  }

  componentDidMount () {
    chrome.windows.getCurrent({}, this.onWindowCallback)
    chrome.braveRewards.getRewardsMainEnabled(((enabled: boolean) => {
      this.props.actions.onEnabledMain(enabled)
    }))
    chrome.braveRewards.getACEnabled(((enabled: boolean) => {
      this.props.actions.onEnabledAC(enabled)
    }))
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (
      !prevProps.rewardsPanelData.walletCreated &&
      this.props.rewardsPanelData.walletCreated
    ) {
      this.getTabData()
    }
  }

  getTabData () {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      const pollTwitchPage = (tab: chrome.tabs.Tab, tabId: number, publisherBlob: string) => {
        // use an interval here to monitor when the DOM has finished
        // generating. clear after the data is present.
        // Check every second no more than 'limit' times
        // clear the interval if panel closes

        const markupMatch = '<figure class=\"tw-avatar tw-avatar--size-36\">' +
                            '<div class=\"tw-border-radius-medium tw-overflow-hidden\">' +
                            '<img class=\"tw-avatar__img tw-image\" alt=\"'
        const notYetRetrievedMatch = 'https://static-cdn.jtvnw.net/jtv_user_pictures/xarth/404_user_70x70.png'
        let itr = 0
        const limit = 10
        let interval = setInterval(poll, 1000)
        function poll () {
          chrome.tabs.executeScript(tabId, {
            code: 'document.body.outerHTML'
          }, function (result: string[]) {
            if (result[0].includes(markupMatch) && !result[0].includes(notYetRetrievedMatch)) {
              publisherBlob = result[0]
              clearInterval(interval)
              const rewardsPanelActions = require('../background/actions/rewardsPanelActions').default
              rewardsPanelActions.onTabRetrieved(tab, publisherBlob, false)
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

                const rewardsPanelActions = require('../background/actions/rewardsPanelActions').default
                rewardsPanelActions.onTabRetrieved(tab, publisherBlob, false)
              }
            }
          })
        }
        poll()
      }

      const pollData = (tab: chrome.tabs.Tab, tabId: number, url: URL) => {
        let publisherBlob = ''
        if (url && url.href.startsWith('https://www.twitch.tv/')) {
          chrome.storage.local.get(['rewards_panel_open'], function (result) {
            if (result['rewards_panel_open'] === 'true') {
              pollTwitchPage(tab, tabId, publisherBlob)
            }
          })
        } else {
          this.props.actions.onTabRetrieved(tab, publisherBlob, false)
        }
      }
      let tab = tabs[0]
      if (tab.url && tab.id) {
        let url = new URL(tab.url)
        if (url && url.host.endsWith('.twitch.tv')) {
          pollData(tab, tab.id, url)
        } else {
          this.props.actions.onTabRetrieved(tab, '', false)
        }
      } else {
        this.props.actions.onTabRetrieved(tab, '', false)
      }
    })
  }

  getGrants () {
    this.props.actions.getGrants()
  }

  onWindowCallback = (window: chrome.windows.Window) => {
    this.setState({
      windowId: window.id
    })

    if (this.props.rewardsPanelData.walletCreated) {
      this.getGrants()
      this.getTabData()
    }
  }

  openRewards () {
    chrome.tabs.create({
      url: 'chrome://rewards'
    })
  }

  get actions () {
    return this.props.actions
  }

  onCreate = () => {
    this.actions.createWallet()
  }

  render () {
    const {
      enabledMain,
      walletCreateFailed,
      walletCreated,
      walletCreating
    } = this.props.rewardsPanelData

    if (!walletCreated) {
      return (
        <PanelWelcome
          error={walletCreateFailed}
          creating={walletCreating}
          variant={'two'}
          optInAction={this.onCreate}
          optInErrorAction={this.onCreate}
          moreLink={this.openRewards}
        />
      )
    }

    return (
      <>
        {
          enabledMain
          ? <Panel windowId={this.state.windowId} />
          : <div style={{ width: '330px' }}>
              <DisabledPanel onLinkOpen={this.openRewards} />
            </div>
        }
      </>
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
