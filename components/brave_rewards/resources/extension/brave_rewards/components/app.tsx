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
} from '../../../ui/components'
import { BatColorIcon, WalletAddIcon } from 'brave-ui/components/icons'
import { WalletState } from '../../../ui/components/walletWrapper'
import { getTabData } from '../background/api/tabs_api'

// Components
import Panel from './panel'

// Utils
import * as utils from '../utils'
import { getMessage } from '../background/api/locale_api'
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
    chrome.braveRewards.getWalletExists((exists: boolean) => {
      this.props.actions.walletExists(exists)
    })

    chrome.braveRewards.isInitialized((initialized: boolean) => {
      if (initialized) {
        this.props.actions.initialized()
      }
    })

    chrome.braveRewards.onlyAnonWallet((only: boolean) => {
      this.setState({
        onlyAnonWallet: !!only
      })
    })

    chrome.braveRewards.getRewardsMainEnabled(((enabled: boolean) => {
      this.props.actions.onEnabledMain(enabled)

      if (enabled && !this.props.rewardsPanelData.initializing) {
        this.startRewards()
      }
    }))
  }

  startRewards = () => {
    this.getCurrentTab(this.onCurrentTab)

    chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
      this.props.actions.onAllNotifications(list)
    })

    const { externalWallet, walletCreated } = this.props.rewardsPanelData

    if (walletCreated) {
      utils.getExternalWallet(this.actions, externalWallet)
      this.getBalance()
      chrome.braveRewards.getRewardsParameters((parameters: RewardsExtension.RewardsParameters) => {
        rewardsPanelActions.onRewardsParameters(parameters)
      })

      chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
        this.props.actions.onAllNotifications(list)
      })

      this.getCurrentTab(this.onCurrentTab)

      this.handleGrantNotification()
    }
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (!this.props.rewardsPanelData.walletCreated) {
      return
    }

    if (
      !prevProps.rewardsPanelData.walletCreated &&
      this.props.rewardsPanelData.walletCreated
    ) {
      this.getTabData()
    }

    if (this.props.rewardsPanelData.enabledMain &&
        prevProps.rewardsPanelData.initializing &&
        !this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }

    if (!prevProps.rewardsPanelData.enabledMain &&
        this.props.rewardsPanelData.enabledMain &&
        !this.props.rewardsPanelData.initializing) {
      this.startRewards()
    }
  }

  getBalance () {
    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      this.actions.onBalance(balance)
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

  goToUphold = () => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet || !externalWallet.accountUrl) {
      this.actions.getExternalWallet('uphold')
      return
    }

    window.open(externalWallet.accountUrl, '_blank')
  }

  onDisconnectClick = () => {
    chrome.braveRewards.disconnectWallet('uphold')
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

    if (this.props.rewardsPanelData.walletCreated) {
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
    this.props.actions.toggleEnableMain(true)
  }

  openRewardsAddFunds = () => {
    const { externalWallet, balance } = this.props.rewardsPanelData

    if (!externalWallet) {
      return
    }

    if (externalWallet.addUrl) {
      chrome.tabs.create({
        url: externalWallet.addUrl
      })
      return
    }

    utils.handleUpholdLink(balance, externalWallet)
  }

  openTOS () {
    chrome.tabs.create({
      url: 'https://basicattentiontoken.org/user-terms-of-service'
    })
  }

  get actions () {
    return this.props.actions
  }

  onCreate = () => {
    this.actions.createWallet()
  }

  getActions = () => {
    let actions = []

    if (!this.state.onlyAnonWallet) {
      actions.push({
        name: getMessage('addFunds'),
        action: this.openRewardsAddFunds,
        icon: <WalletAddIcon />,
        externalWallet: true
      })
    }

    return actions.concat([{
      name:  getMessage('rewardsSettings'),
      action: this.openRewards,
      icon: <BatColorIcon />,
      externalWallet: false
    }])
  }

  render () {
    const {
      enabledMain,
      walletCreateFailed,
      walletCreated,
      walletCreating,
      walletCorrupted,
      balance,
      externalWallet,
      parameters
    } = this.props.rewardsPanelData

    const total = balance.total || 0
    const converted = utils.convertBalance(total, parameters.rate)

    if (!walletCreated || walletCorrupted) {
      return (
        <div data-test-id={'rewards-panel'}>
          <PanelWelcome
            error={walletCreateFailed}
            creating={walletCreating}
            variant={'two'}
            optInAction={this.onCreate}
            optInErrorAction={this.onCreate}
            moreLink={this.openRewards}
            onTOSClick={this.openTOS}
            onlyAnonWallet={this.state.onlyAnonWallet}
            onPrivacyClick={this.openPrivacyPolicy}
          />
        </div>
      )
    }

    let walletStatus: WalletState | undefined = undefined
    let onVerifyClick = undefined
    if (!this.state.onlyAnonWallet) {
      walletStatus = utils.getWalletStatus(externalWallet)
      onVerifyClick = utils.handleUpholdLink.bind(this, balance, externalWallet)
    }

    return (
      <div data-test-id={'rewards-panel'}>
        {
          enabledMain
          ? <Panel
              tabId={this.state.tabId}
              onlyAnonWallet={this.state.onlyAnonWallet}
          />
          : <>
              <WalletWrapper
                compact={true}
                contentPadding={false}
                gradientTop={'249,251,252'}
                balance={total.toFixed(3)}
                showSecActions={false}
                showCopy={false}
                onlyAnonWallet={this.state.onlyAnonWallet}
                converted={utils.formatConverted(converted)}
                walletState={walletStatus}
                onVerifyClick={onVerifyClick}
                onDisconnectClick={this.onDisconnectClick}
                goToUphold={this.goToUphold}
                greetings={utils.getGreetings(externalWallet)}
                actions={this.getActions()}
              >
                <WalletPanelDisabled
                  onTOSClick={this.openTOS}
                  onEnable={this.enableRewards}
                  onPrivacyClick={this.openPrivacyPolicy}
                />
              </WalletWrapper>
            </>
        }
      </div>
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
