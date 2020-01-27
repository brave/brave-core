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
  onlyAnonWallet: boolean
}

export class RewardsPanel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      windowId: -1,
      onlyAnonWallet: false
    }
  }

  componentDidMount () {
    chrome.braveRewards.onlyAnonWallet((only: boolean) => {
      this.setState({
        onlyAnonWallet: !!only
      })
    })

    chrome.windows.getCurrent({}, this.onWindowCallback)

    chrome.braveRewards.getRewardsMainEnabled(((enabled: boolean) => {
      this.props.actions.onEnabledMain(enabled)
    }))

    chrome.braveRewards.getAllNotifications((list: RewardsExtension.Notification[]) => {
      this.props.actions.onAllNotifications(list)
    })

    this.handleGrantNotification()

    const { externalWallet, walletCreated } = this.props.rewardsPanelData

    if (walletCreated) {
      utils.getExternalWallet(this.actions, externalWallet)
      this.getBalance()
    }
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (
      !prevProps.rewardsPanelData.walletCreated &&
      this.props.rewardsPanelData.walletCreated
    ) {
      this.getTabData()
    }
    if (!prevProps.rewardsPanelData.enabledMain && this.props.rewardsPanelData.enabledMain) {
      chrome.windows.getCurrent({}, this.onWindowCallback)
      this.getBalance()
    }
  }

  getBalance () {
    chrome.braveRewards.fetchBalance((balance: RewardsExtension.Balance) => {
      this.actions.onBalance(balance)
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

                const rewardsPanelActions = require('../background/actions/rewardsPanelActions').default
                rewardsPanelActions.onTabRetrieved(tab)
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
          this.props.actions.onTabRetrieved(tab)
        }
      }
      let tab = tabs[0]
      if (tab.url && tab.id) {
        let url = new URL(tab.url)
        if (url && url.host.endsWith('.twitch.tv')) {
          pollData(tab, tab.id, url)
        } else {
          this.props.actions.onTabRetrieved(tab)
        }
      } else {
        this.props.actions.onTabRetrieved(tab)
      }
    })
  }

  onWindowCallback = (window: chrome.windows.Window) => {
    this.setState({
      windowId: window.id
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
    this.props.actions.onSettingSave('enabledMain', '1')
  }

  openRewardsAddFunds = () => {
    const { externalWallet } = this.props.rewardsPanelData

    if (!externalWallet) {
      return
    }

    if (externalWallet.addUrl) {
      chrome.tabs.create({
        url: externalWallet.addUrl
      })
      return
    }

    if (externalWallet.verifyUrl) {
      utils.handleUpholdLink(externalWallet.verifyUrl, externalWallet)
      return
    }
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

  getActions = () => {
    let actions = []

    if (!this.state.onlyAnonWallet) {
      actions.push({
        name: getMessage('addFunds'),
        action: this.openRewardsAddFunds,
        icon: <WalletAddIcon />
      })
    }

    return actions.concat([{
      name:  getMessage('rewardsSettings'),
      action: this.openRewards,
      icon: <BatColorIcon />
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
      promotions
    } = this.props.rewardsPanelData

    const total = balance.total || 0
    const converted = utils.convertBalance(total, balance.rates)
    const claimedPromotions = utils.getClaimedPromotions(promotions || [])

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
      onVerifyClick = utils.onVerifyClick.bind(this, this.actions)
    }

    return (
      <div data-test-id={'rewards-panel'}>
        {
          enabledMain
          ? <Panel
              windowId={this.state.windowId}
              onlyAnonWallet={this.state.onlyAnonWallet}
          />
          : <>
              <WalletWrapper
                compact={true}
                contentPadding={false}
                gradientTop={'249,251,252'}
                balance={total.toFixed(1)}
                showSecActions={false}
                showCopy={false}
                onlyAnonWallet={this.state.onlyAnonWallet}
                grants={utils.generatePromotions(claimedPromotions)}
                converted={utils.formatConverted(converted)}
                walletState={walletStatus}
                onVerifyClick={onVerifyClick}
                onDisconnectClick={this.onDisconnectClick}
                goToUphold={this.goToUphold}
                userName={utils.getUserName(externalWallet)}
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
