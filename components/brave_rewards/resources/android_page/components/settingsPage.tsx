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
import { Promo } from '../../android_page/components/promo'

import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'
import MonthlyTipsBox from './monthlyTipsBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Promotion from './promotion'
import { getLocale } from '../../../../common/locale'
import { getActivePromos, getPromo, PromoType } from '../../page/components/promos'
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

  onDismissPromo = (promo: PromoType) => {
    this.actions.dismissPromoPrompt(promo)
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

    if (externalWallet && externalWallet.loginUrl) {
      window.open(externalWallet.loginUrl, '_self')
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
            errorText={[getLocale('redirectModalDeviceLimitReachedText')]}
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
            errorText={[getLocale('redirectModalError')]}
            buttonText={getLocale('processingRequestButton')}
            titleText={getLocale('processingRequest')}
            walletType={walletType}
            displayCloseButton={true}
            isMobile={true}
            onClick={this.onRedirectError}
            onClose={this.actions.hideRedirectModal}
          />
        )
      case 'flaggedWalletModal':
        return (
          <ModalRedirect
            id={'redirect-modal-flagged-wallet'}
            errorText={[
              getLocale('redirectModalFlaggedWalletText1'),
              getLocale('redirectModalFlaggedWalletText2'),
              getLocale('redirectModalFlaggedWalletText3'),
              getLocale('redirectModalFlaggedWalletText4')]}
            errorTextLink={'https://support.brave.com/hc/en-us/articles/4494596374925'}
            titleText={getLocale('redirectModalFlaggedWalletTitle')}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'kycRequiredModal':
        return (
          <ModalRedirect
            id={'redirect-modal-id-verification-required'}
            titleText={getLocale('redirectModalKYCRequiredTitle')}
            errorText={[getLocale('redirectModalKYCRequiredText').replace('$1', getWalletProviderName(externalWallet))]}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'mismatchedProviderAccountRegionsModal':
        return (
          <ModalRedirect
            id={'redirect-modal-mismatched-provider-account-regions'}
            errorText={[getLocale('redirectModalMismatchedProviderAccountRegionsText')]}
            titleText={getLocale('redirectModalMismatchedProviderAccountRegionsTitle')}
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
            errorText={[getLocale('redirectModalMismatchedProviderAccountsText').replace('$1', getWalletProviderName(externalWallet))]}
            titleText={getLocale('redirectModalMismatchedProviderAccountsTitle')}
            learnMore={'https://support.brave.com/hc/en-us/articles/360034841711-What-is-a-verified-wallet-'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'regionNotSupportedModal':
        return (
          <ModalRedirect
            id={'redirect-modal-region-not-supported'}
            errorText={[
              getLocale('redirectModalRegionNotSupportedText1').replaceAll('$1', getWalletProviderName(externalWallet)),
              getLocale('redirectModalRegionNotSupportedText2')]}
            titleText={getLocale('redirectModalRegionNotSupportedTitle')}
            errorTextLink={'https://support.brave.com/hc/en-us/articles/6539887971469'}
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
            errorText={[getLocale('redirectModalUpholdBATNotAllowedText')]}
            titleText={getLocale('redirectModalUpholdBATNotAllowedTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360033020351-Brave-BAT-and-US-availability'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            isMobile={true}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdInsufficientCapabilitiesModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-insufficient-capabilities'}
            errorText={[getLocale('redirectModalUpholdInsufficientCapabilitiesText')]}
            titleText={getLocale('redirectModalUpholdInsufficientCapabilitiesTitle')}
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

  renderPromos = () => {
    const { currentCountryCode, externalWallet, ui } = this.props.rewardsData
    const { promosDismissed } = ui

    return (
      <>
        {getActivePromos(externalWallet, true).map((key: PromoType) => {
           if (promosDismissed && promosDismissed[key]) {
             return null
           }

           const promo = getPromo(key)
           if (!promo) {
             return null
           }

           const { supportedLocales } = promo
           if (supportedLocales && supportedLocales.length && !supportedLocales.includes(currentCountryCode)) {
             return null
           }

           return (
             <Promo
               key={`${key}-promo`}
               title={promo.title}
               link={promo.link}
               copy={promo.copy}
               onDismissPromo={this.onDismissPromo.bind(this, key)}
             />
           )
        })}
      </>
    )
  }

  renderSettings () {
    return (
      <>
        <MainToggleMobile />
        <AdsBox />
        <ContributeBox />
        <TipBox />
        <MonthlyTipsBox />
      </>
    )
  }

  render () {
    return (
      <Page>
        {this.getPromotionsClaims()}
        <PageWallet showManageWalletButton={false} />
        {this.getRedirectModal()}
        {this.renderPromos()}
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
