/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { WalletAddIcon, BatColorIcon } from 'brave-ui/components/icons'
import { WalletWrapper, WalletSummary, WalletSummarySlider, WalletPanel } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'
import { NotificationType } from 'brave-ui/features/rewards/walletWrapper'
import { Type as AlertType } from 'brave-ui/features/rewards/alert'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'
import * as utils from '../utils'

import { getMessage } from '../background/api/locale_api'

interface Props extends RewardsExtension.ComponentProps {
  windowId: number
}

interface State {
  showSummary: boolean
  publisherKey: string | null
}

export class Panel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showSummary: true,
      publisherKey: null
    }
  }

  get actions () {
    return this.props.actions
  }

  componentDidMount () {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisher_key

    if (newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey
      })
    }

    this.actions.getWalletProperties()
    this.actions.getCurrentReport()
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisher_key

    if (!prevState.publisherKey && newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey
      })
    } else if (prevState.publisherKey && !newKey) {
      this.setState({
        showSummary: true,
        publisherKey: null
      })
    }
  }

  get gradientColor () {
    return this.state.showSummary ? '233,235,255' : '249,251,252'
  }

  doNothing () {
    console.log('doNothing click')
  }

  switchAutoContribute = () => {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const publisherKey = publisher && publisher.publisher_key
    const excluded = publisher && publisher.excluded
    if (publisherKey && publisherKey.length > 0 && excluded !== undefined) {
      this.props.actions.includeInAutoContribution(publisherKey, !excluded, this.props.windowId)
    }
  }

  getPublisher = () => {
    let windowId = this.props.windowId.toString()

    if (!windowId) {
      return undefined
    }

    windowId = `id_${windowId}`

    return this.props.rewardsPanelData.publishers[windowId]
  }

  onSliderToggle = () => {
    this.setState({
      showSummary: !this.state.showSummary
    })
  }

  getGrants = (grants?: RewardsExtension.Grant[]) => {
    if (!grants) {
      return []
    }

    return grants.map((grant: RewardsExtension.Grant) => {
      return {
        tokens: utils.convertProbiToFixed(grant.probi),
        expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString()
      }
    })
  }

  getWalletSummary = () => {
    const { walletProperties, report } = this.props.rewardsPanelData
    const { rates } = walletProperties

    let props = {}

    if (report) {
      for (let key in report) {
        const item = report[key]

        if (item.length > 1 && key !== 'total') {
          const tokens = utils.convertProbiToFixed(item)
          props[key] = {
            tokens,
            converted: utils.convertBalance(tokens, rates)
          }
        }
      }
    }

    return {
      report: props
    }
  }

  openRewardsPage () {
    chrome.tabs.create({
      url: 'chrome://rewards'
    })
  }

  openRewardsAddFundsPage () {
    chrome.tabs.create({
      url: 'brave://rewards/#add-funds'
    })
  }

  showDonateToSiteDetail = () => {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    // TODO: why do we store windowId instead of active tab id in state?
    chrome.tabs.query({
      active: true,
      windowId: chrome.windows.WINDOW_ID_CURRENT
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      const tabId = tabs[0].id
      if (tabId === undefined || !publisher) {
        return
      }
      chrome.braveRewards.donateToSite(tabId, publisher.publisher_key)
      window.close()
    })
  }

  onCloseNotification = (id: string) => {
    this.actions.deleteNotification(id)
  }

  getNotification = () => {
    const { notifications, currentNotification } = this.props.rewardsPanelData

    if (
      currentNotification === undefined ||
      !notifications ||
      (currentNotification && !notifications[currentNotification])
    ) {
      return undefined
    }

    const notification: RewardsExtension.Notification = notifications[currentNotification]

    let type: NotificationType = ''
    let text = ''
    let isAlert = ''
    switch (notification.type) {
      case 1:
        {
          if (!notification.args ||
            !Array.isArray(notification.args) ||
            notification.args.length < 3) {
            break
          }

          const result = notification.args[1]

          // Results
          // 0 - success
          // 1 - general error
          // 15 - not enough funds
          // 16 - error while tipping

          if (result === '0') {
            const fixed = utils.convertProbiToFixed(notification.args[3])
            text = getMessage('contributeNotificationSuccess', [fixed])
          } else if (result === '15') {
            text = getMessage('contributeNotificationNotEnoughFunds')
            isAlert = 'warning'
          } else if (result === '16') {
            text = getMessage('contributeNotificationTipError')
            isAlert = 'error'
          } else {
            text = getMessage('contributeNotificationError')
            isAlert = 'error'
          }
          type = 'contribute'
          break
        }
      case 2:
        type = 'grant'
        text = getMessage('grantNotification')
        break
    }

    if (isAlert) {
      return {
        alert: {
          node: text,
          type: isAlert as AlertType,
          onAlertClose: this.onCloseNotification.bind(this, notification.id)
        }
      }
    }

    return {
      notification: {
        id: notification.id,
        date: new Date(notification.timestamp * 1000).toLocaleDateString(),
        type: type,
        text: text,
        onCloseNotification: this.onCloseNotification
      }
    }
  }

  render () {
    const { balance, rates, grants } = this.props.rewardsPanelData.walletProperties
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const converted = utils.convertBalance(balance.toString(), rates)
    const notification = this.getNotification()

    let faviconUrl
    if (publisher && publisher.url) {
      faviconUrl = `chrome://favicon/size/48@2x/${publisher.url}`
      if (publisher.favicon_url) {
        faviconUrl = `chrome://favicon/size/48@2x/${publisher.favicon_url}`
      }
    }
    return (
      <WalletWrapper
        compact={true}
        contentPadding={false}
        gradientTop={this.gradientColor}
        balance={balance.toFixed(1)}
        converted={utils.formatConverted(converted)}
        actions={[
          {
            name: 'Add funds',
            action: this.openRewardsAddFundsPage,
            icon: <WalletAddIcon />
          },
          {
            name: 'Rewards Settings',
            action: this.openRewardsPage,
            icon: <BatColorIcon />
          }
        ]}
        showCopy={false}
        showSecActions={false}
        connectedWallet={false}
        grants={this.getGrants(grants)}
        {...notification}
      >
        <WalletSummarySlider
          id={'panel-slider'}
          onToggle={this.onSliderToggle}
        >
          {
            publisher && publisher.publisher_key
            ? <WalletPanel
              id={'wallet-panel'}
              platform={publisher.provider as Provider}
              publisherName={publisher.name}
              publisherImg={faviconUrl}
              monthlyAmount={10}
              isVerified={publisher.verified}
              tipsEnabled={true}
              includeInAuto={!publisher.excluded}
              attentionScore={(publisher.percentage || 0).toString()}
              onToggleTips={this.doNothing}
              donationAction={this.showDonateToSiteDetail}
              onAmountChange={this.doNothing}
              onIncludeInAuto={this.switchAutoContribute}
            />
            : null
          }
          <WalletSummary compact={true} {...this.getWalletSummary()}/>
        </WalletSummarySlider>
      </WalletWrapper>
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
)(Panel)
