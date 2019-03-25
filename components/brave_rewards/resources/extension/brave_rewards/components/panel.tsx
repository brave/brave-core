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
import { RewardsNotificationType } from '../constants/rewards_panel_types'
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
  newMonthlyAmount: string | null
}

export class Panel extends React.Component<Props, State> {
  private defaultDonationAmounts: number[]

  constructor (props: Props) {
    super(props)
    this.state = {
      showSummary: true,
      publisherKey: null,
      newMonthlyAmount: null
    }
    this.defaultDonationAmounts = [0, 1, 5, 10]
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

    chrome.braveRewards.getPendingContributionsTotal(((amount: number) => {
      this.actions.OnPendingContributionsTotal(amount)
    }))
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
      this.props.actions.includeInAutoContribution(publisherKey, !excluded)
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

  onFetchCaptcha = () => {
    let notification: any = this.getNotification()

    if (!notification || (notification && notification.alert)) {
      return
    }

    notification = notification && notification.notification

    if (notification && notification.id) {
      const split = notification.id.split('_')
      const promoId = split[split.length - 1]
      if (promoId) {
        this.actions.getGrantCaptcha(promoId)
      }
    }
  }

  onBackupWallet = (id: string) => {
    chrome.tabs.create({
      url: 'chrome://rewards#backup-restore'
    })
    this.actions.deleteNotification(id)
  }

  onGrantHide = () => {
    this.actions.onResetGrant()
  }

  onFinish = () => {
    this.actions.onDeleteGrant()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solveGrantCaptcha(x, y)
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
      url: 'chrome://rewards/#add-funds'
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
      if (tabId === undefined || !publisher || !publisher.publisher_key) {
        return
      }

      chrome.braveRewards.donateToSite(tabId, publisher.publisher_key)
      window.close()
    })
  }

  onCloseNotification = (id: string) => {
    this.actions.deleteNotification(id)
  }

  getNotificationProp = (key: string, notification: any) => {
    if (!notification ||
        !notification.notification ||
        !notification.notification[key]) {
      return null
    }

    return notification.notification[key]
  }

  getNotificationClickEvent = (type?: string, id?: string) => {
    if (!type) {
      return undefined
    }

    let clickEvent

    switch (type) {
      case 'grant':
        clickEvent = this.onFetchCaptcha
        break
      case 'backupWallet':
        clickEvent = this.onBackupWallet.bind(this, id)
        break
      default:
        clickEvent = undefined
        break
    }

    return clickEvent
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
      case RewardsNotificationType.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE: {
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
      case RewardsNotificationType.REWARDS_NOTIFICATION_GRANT:
        type = 'grant'
        text = getMessage('grantNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_GRANT_ADS:
        type = 'grant'
        text = getMessage('earningsClaimDefault')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_BACKUP_WALLET:
        type = 'backupWallet'
        text = getMessage('backupWalletNotification')
        break
      case RewardsNotificationType.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
        type = 'insufficientFunds'
        text = getMessage('insufficientFundsNotification')
        break
      default:
        type = ''
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

  onContributionAmountChange = (publisherKey: string, newAmount: string) => {
    let newMonthlyAmount: string
    const newValue = parseInt(newAmount, 10)

    if (newValue === 0) {
      newMonthlyAmount = '0.0'
      this.actions.removeRecurringContribution(publisherKey)
    } else {
      newMonthlyAmount = newValue.toFixed(1).toString()
      this.actions.saveRecurringDonation(publisherKey, newValue)
    }

    this.setState({
      newMonthlyAmount: newMonthlyAmount
    })
  }

  generateAmounts = (publisher?: RewardsExtension.Publisher) => {
    const { donationAmounts } = this.props.rewardsPanelData
    const { rates } = this.props.rewardsPanelData.walletProperties

    const publisherKey = publisher && publisher.publisher_key

    const amounts = (!publisherKey || !donationAmounts || !donationAmounts[publisherKey])
      ? this.defaultDonationAmounts
      : donationAmounts[publisherKey]

    return amounts.map((value: number) => {
      return {
        tokens: value.toFixed(1),
        converted: utils.convertBalance(value.toString(), rates),
        selected: false
      }
    })
  }

  getContribution = (publisher?: RewardsExtension.Publisher) => {
    let defaultContribution = '0.0'
    const { recurringDonations } = this.props.rewardsPanelData

    if (!recurringDonations ||
       (!publisher || !publisher.publisher_key)) {
      return defaultContribution
    }

    recurringDonations.map((donation: any) => {
      if (donation.publisherKey === publisher.publisher_key) {
        defaultContribution = donation.amount.toFixed(1)
      }
    })

    return defaultContribution
  }

  render () {
    const { pendingContributionTotal, enabledAC } = this.props.rewardsPanelData
    const { balance, rates, grants } = this.props.rewardsPanelData.walletProperties
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const converted = utils.convertBalance(balance.toString(), rates)
    const notification = this.getNotification()
    const notificationId = this.getNotificationProp('id', notification)
    const notificationType = this.getNotificationProp('type', notification)
    const notificationClick = this.getNotificationClickEvent(notificationType, notificationId)
    const { currentGrant } = this.props.rewardsPanelData
    const defaultContribution = this.getContribution(publisher)
    const donationAmounts = defaultContribution !== '0.0'
      ? this.generateAmounts(publisher)
      : undefined

    const pendingTotal = parseFloat(
      (pendingContributionTotal || 0).toFixed(1))

    let faviconUrl
    if (publisher && publisher.url) {
      faviconUrl = `chrome://favicon/size/48@2x/${publisher.url}`
      if (publisher.favicon_url && publisher.verified) {
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
            name: getMessage('addFunds'),
            action: this.openRewardsAddFundsPage,
            icon: <WalletAddIcon />
          },
          {
            name:  getMessage('rewardsSettings'),
            action: this.openRewardsPage,
            icon: <BatColorIcon />
          }
        ]}
        showCopy={false}
        showSecActions={false}
        connectedWallet={false}
        grant={currentGrant}
        onGrantHide={this.onGrantHide}
        onNotificationClick={notificationClick}
        onSolution={this.onSolution}
        onFinish={this.onFinish}
        convertProbiToFixed={utils.convertProbiToFixed}
        grants={utils.getGrants(grants)}
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
              monthlyAmount={this.state.newMonthlyAmount || defaultContribution}
              isVerified={publisher.verified}
              tipsEnabled={true}
              includeInAuto={!publisher.excluded}
              attentionScore={(publisher.percentage || 0).toString()}
              onToggleTips={this.doNothing}
              donationAction={this.showDonateToSiteDetail}
              onAmountChange={this.onContributionAmountChange.bind(this, publisher.publisher_key)}
              onIncludeInAuto={this.switchAutoContribute}
              showUnVerified={!publisher.verified}
              acEnabled={enabledAC}
              donationAmounts={donationAmounts}
              moreLink={'https://brave.com/faq-rewards/#unclaimed-funds'}
            />
            : null
          }
          <WalletSummary
            compact={true}
            reservedAmount={pendingTotal}
            reservedMoreLink={'https://brave.com/faq-rewards/#unclaimed-funds'}
            {...this.getWalletSummary()}
          />
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
