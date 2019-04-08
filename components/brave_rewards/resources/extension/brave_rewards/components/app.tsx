/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import {
  PanelWelcome,
  WalletPanelDisabled,
  WalletWrapper
} from 'brave-ui/features/rewards'
import { BatColorIcon, WalletAddIcon } from 'brave-ui/components/icons'

// Components
import Panel from './panel'

// Utils
import * as utils from '../utils'
import { getMessage } from '../background/api/locale_api'
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
              rewardsPanelActions.onTabRetrieved(tab, publisherBlob)
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
                rewardsPanelActions.onTabRetrieved(tab, publisherBlob)
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
          this.props.actions.onTabRetrieved(tab, publisherBlob)
        }
      }
      let tab = tabs[0]
      if (tab.url && tab.id) {
        let url = new URL(tab.url)
        if (url && url.host.endsWith('.twitch.tv')) {
          pollData(tab, tab.id, url)
        } else {
          this.props.actions.onTabRetrieved(tab, '')
        }
      } else {
        this.props.actions.onTabRetrieved(tab, '')
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

  openPrivacyPolicy () {
    chrome.tabs.create({
      url: 'https://brave.com/privacy#rewards'
    })
  }

  openRewards () {
    chrome.tabs.create({
      url: 'chrome://rewards'
    })
  }

  enableRewards = () => {
    this.props.actions.onSettingSave('enabledMain', '1')
  }

  openRewardsAddFunds () {
    chrome.tabs.create({
      url: 'chrome://rewards/#add-funds'
    })
  }

  openTOS () {
    chrome.tabs.create({
      url: 'https://brave.com/terms-of-use'
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
      walletCreating,
      walletProperties
    } = this.props.rewardsPanelData

    const { balance, grants, rates } = walletProperties
    const converted = utils.convertBalance(balance.toString(), rates)

    if (!walletCreated) {
      return (
        <PanelWelcome
          error={walletCreateFailed}
          creating={walletCreating}
          variant={'two'}
          optInAction={this.onCreate}
          optInErrorAction={this.onCreate}
          moreLink={this.openRewards}
          onTOSClick={this.openTOS}
          onPrivacyClick={this.openPrivacyPolicy}
        />
      )
    }

    return (
      <>
        {
          enabledMain
          ? <Panel windowId={this.state.windowId} />
          : <>
              <WalletWrapper
                compact={true}
                contentPadding={false}
                gradientTop={'249,251,252'}
                balance={balance.toFixed(1)}
                showSecActions={false}
                connectedWallet={false}
                showCopy={false}
                grants={utils.getGrants(grants)}
                converted={utils.formatConverted(converted)}
                convertProbiToFixed={utils.convertProbiToFixed}
                actions={[
                  {
                    name: getMessage('addFunds'),
                    action: this.openRewardsAddFunds,
                    icon: <WalletAddIcon />
                  },
                  {
                    name:  getMessage('rewardsSettings'),
                    action: this.openRewards,
                    icon: <BatColorIcon />
                  }
                ]}
              >
                <WalletPanelDisabled
                  onTOSClick={this.openTOS}
                  onEnable={this.enableRewards}
                  onPrivacyClick={this.openPrivacyPolicy}
                />
              </WalletWrapper>
            </>
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
