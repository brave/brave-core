/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  MainToggleMobile,
  SettingsPageMobile as Page
} from '../../ui/components/mobile'
import { ModalRedirect } from '../../ui/components'

import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'
import MonthlyContributionBox from './monthlyContributionBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Promotion from './promotion'
import { getLocale } from '../../../../common/locale'
import { getWalletProviderName } from '../utils'

export interface Props extends Rewards.ComponentProps {
}

interface State {
  redirectModalDisplayed: 'hide' | 'show'
  showRewardsTour: boolean
  firstTimeSetup: boolean
}

class SettingsPage extends React.Component<Props, State> {
  private balanceTimerId: number

  constructor (props: Props) {
    super(props)
    this.state = {
      redirectModalDisplayed: 'hide',
      showRewardsTour: false,
      firstTimeSetup: false
    }
  }

  get actions () {
    return this.props.actions
  }

  refreshActions () {
    this.actions.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear())
    this.actions.getTipTable()
    this.actions.getContributeList()
    this.actions.getPendingContributions()
    this.actions.getReconcileStamp()
    this.actions.getStatement()
    this.actions.getAdsData()
    this.actions.getExcludedSites()
    this.actions.getCountryCode()
  }

  componentDidMount () {
    if (!this.props.rewardsData.initializing) {
      this.startRewards()
    }
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (
      prevProps.rewardsData.initializing &&
      !this.props.rewardsData.initializing
    ) {
      this.startRewards()
    }

    if (
      !prevProps.rewardsData.enabledContribute &&
      this.props.rewardsData.enabledContribute
    ) {
      this.actions.getContributeList()
      this.actions.getReconcileStamp()
    }

    if (
      prevState.redirectModalDisplayed !== 'hide' &&
      this.props.rewardsData.ui.modalRedirect === 'hide'
    ) {
      this.setState({
        redirectModalDisplayed: 'hide'
      })
      window.history.replaceState({}, 'Rewards', '/')
    } else if (
      prevState.redirectModalDisplayed === 'hide' &&
      this.props.rewardsData.ui.modalRedirect !== 'hide'
    ) {
      this.setState({
        redirectModalDisplayed: 'show'
      })
    }

    if (
      prevProps.rewardsData.externalWallet &&
      !this.props.rewardsData.externalWallet
    ) {
      this.actions.getExternalWallet()
    }
  }

  stopRewards () {
    window.clearInterval(this.balanceTimerId)
    this.balanceTimerId = -1
  }

  startRewards () {
    this.refreshActions()

    this.actions.getRewardsParameters()
    this.actions.getContributionAmount()
    this.actions.getAutoContributeProperties()
    this.actions.getBalance()
    this.balanceTimerId = window.setInterval(() => {
      this.actions.getBalance()
    }, 60000)

    this.actions.fetchPromotions()
    this.actions.getExternalWallet()
    this.actions.getOnboardingStatus()
    this.actions.getEnabledInlineTippingPlatforms()

    this.handleURL()
  }

  handleURL () {
    const { pathname } = window.location

    if (pathname === '/enable') {
      this.actions.saveOnboardingResult('opted-in')
      this.setState({ showRewardsTour: true, firstTimeSetup: true })
      window.history.replaceState({}, '', '/')
      return
    }

    if (pathname.length > 1) {
      const pathElements = pathname.split('/')
      if (pathElements.length > 2) {
        this.actions.processRewardsPageUrl(window.location.pathname, window.location.search)
      }
    }
  }

  getPromotionsClaims = () => {
    const { promotions } = this.props.rewardsData

    if (!promotions || promotions.length === 0) {
      return null
    }

    return (
      <div style={{ width: '100%' }} data-test-id={'promotion-claim-box'}>
        {promotions.map((promotion?: Rewards.Promotion, index?: number) => {
          if (!promotion || !promotion.promotionId) {
            return null
          }

          return (
            <div key={`promotion-${index}`}>
              <Promotion promotion={promotion} />
            </div>
          )
        })}
      </div>
    )
  }

  componentWillUnmount () {
    this.stopRewards()
  }

  onRedirectError = () => {
    this.actions.hideRedirectModal()

    const { externalWallet } = this.props.rewardsData

    if (externalWallet && externalWallet.verifyUrl) {
      window.open(externalWallet.verifyUrl, '_self')
    }
  }

  getRedirectModal = () => {
    const { externalWallet, ui } = this.props.rewardsData
    const walletType = externalWallet ? externalWallet.type : ''

    switch (ui.modalRedirect) {
      case 'deviceLimitReachedModal':
        return (
          <ModalRedirect
            id={'redirect-modal-device-limit-reached'}
            errorText={getLocale('redirectModalDeviceLimitReachedText')}
            titleText={getLocale('redirectModalDeviceLimitReachedTitle')}
            learnMore={'https://support.brave.com/hc/en-us/articles/360056508071'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'error':
        return (
          <ModalRedirect
            id={'redirect-modal-error'}
            errorText={getLocale('redirectModalError')}
            buttonText={getLocale('processingRequestButton')}
            titleText={getLocale('processingRequest')}
            walletType={walletType}
            displayCloseButton={true}
            isMobile={true}
            onClick={this.onRedirectError}
            onClose={this.actions.hideRedirectModal}
          />
        )
      case 'kycRequiredModal':
        return (
          <ModalRedirect
            id={'redirect-modal-id-verification-required'}
            titleText={getLocale('redirectModalKYCRequiredTitle')}
            errorText={getLocale('redirectModalKYCRequiredText').replace('$1', getWalletProviderName(externalWallet))}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'mismatchedProviderAccountsModal':
        return (
          <ModalRedirect
            id={'redirect-modal-mismatched-provider-accounts'}
            errorText={getLocale('redirectModalMismatchedProviderAccountsText').replace('$1', getWalletProviderName(externalWallet))}
            titleText={getLocale('redirectModalMismatchedProviderAccountsTitle')}
            learnMore={'https://support.brave.com/hc/en-us/articles/360034841711-What-is-a-verified-wallet-'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'show':
        return (
          <ModalRedirect
            id={'redirect-modal-show'}
            titleText={getLocale('processingRequest')}
            walletType={walletType}
            isMobile={true}
          />
        )
      case 'upholdBATNotAllowedModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-bat-not-allowed'}
            errorText={getLocale('redirectModalUpholdBATNotAllowedText')}
            titleText={getLocale('redirectModalUpholdBATNotAllowedTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360033020351-Brave-BAT-and-US-availability'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdBlockedUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-blocked-user'}
            errorText={getLocale('redirectModalUpholdBlockedUserText')}
            titleText={getLocale('redirectModalUpholdBlockedUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdPendingUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-pending-user'}
            errorText={getLocale('redirectModalUpholdPendingUserText')}
            titleText={getLocale('redirectModalUpholdPendingUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/206695986-How-do-I-sign-up-for-Uphold-Web-'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdRestrictedUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-restricted-user'}
            errorText={getLocale('redirectModalUpholdRestrictedUserText')}
            titleText={getLocale('redirectModalUpholdRestrictedUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      default:
        return null
    }
  }

  renderSettings () {
    return (
      <>
        <MainToggleMobile />
        <AdsBox />
        <ContributeBox />
        <MonthlyContributionBox />
        <TipBox />
      </>
    )
  }

  render () {
    return (
      <Page>
        {this.getPromotionsClaims()}
        <PageWallet />
        {this.getRedirectModal()}
        {this.renderSettings()}
      </Page>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(SettingsPage)
