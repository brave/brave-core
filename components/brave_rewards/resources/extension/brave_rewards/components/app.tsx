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

    if (this.props.rewardsPanelData.pendingPublisherData) {
      const { tabUrl, tabFaviconUrl, windowId, tabId } = this.props.rewardsPanelData.pendingPublisherData
      if (tabUrl && windowId) {
        if (utils.isTwitchUrl(tabUrl) && tabId) {
          this.pollTwitchPage(tabUrl, tabFaviconUrl || '', tabId, windowId)
        } else {
          this.getPublisherData(tabUrl, tabFaviconUrl || '', windowId)
        }
      }
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
      rewardsPanelActions.onTabRetrieved(tabs[0], false)
    })
  }

  poll = (interval: ReturnType<typeof setTimeout>, tabId: number, tabUrl: string, tabFaviconUrl: string, windowId: number) => {
    const markupMatch = 'channel-header'
    let itr = 0
    const limit = 5
    chrome.tabs.executeScript(tabId, {
      code: 'document.body.outerHTML'
    }, (result: string[]) => {
      if (result[0].includes(markupMatch)) {
        clearInterval(interval)
        this.getPublisherData(tabUrl, tabFaviconUrl, windowId, result[0])
      } else {
        itr++
        if (itr === limit) {
          // give up
          clearInterval(interval)
          this.getPublisherData(tabUrl, tabFaviconUrl, windowId)
        }
      }
    })
  }

  pollTwitchPage = (tabUrl: string, tabFaviconUrl: string, tabId: number, windowId: number) => {
    // use an interval here to monitor when the DOM has finished
    // generating. clear after the data is present.
    // Check every second no more than 'limit' times
    // clear the interval if panel closes
    let interval = setInterval(this.poll, 1000)
    this.poll(interval, tabId, tabUrl, tabFaviconUrl, windowId)
  }

  getPublisherData = (tabUrl: string, tabFaviconUrl: string, windowId: number, publisherBlob: string = 'ignore') => {
    chrome.braveRewards.getPublisherData(
    tabUrl,
    tabFaviconUrl || '',
    publisherBlob, (result: RewardsExtension.Result, publisher: RewardsExtension.Publisher) => {
      if (result === RewardsExtension.Result.LEDGER_OK) {
        this.props.actions.onPublisherData(windowId, publisher)
      }
      if (publisher && publisher.publisher_key) {
        chrome.braveRewards.getPublisherBanner(publisher.publisher_key, ((banner: RewardsExtension.PublisherBanner) => {
          rewardsPanelActions.onPublisherBanner(banner)
        }))
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
