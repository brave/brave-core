/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Column, Grid } from 'brave-ui/components'
import {
  MainToggle,
  SettingsPage as Page,
  ModalRedirect,
  SidebarPromo
} from '../../ui/components'
import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'
import MonthlyTipsBox from './monthlyTipsBox'
import { SettingsOptInForm, RewardsTourModal, RewardsTourPromo } from '../../shared/components/onboarding'
import { TourPromoWrapper } from './style'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Promotion from './promotion'
import { getLocale } from '../../../../common/locale'
import { getActivePromos, getPromo, PromoType, Promo } from '../promos'
import { getWalletProviderName } from '../utils'

interface Props extends Rewards.ComponentProps {
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

  openTOS () {
    window.open('https://basicattentiontoken.org/user-terms-of-service', '_blank')
  }

  openPrivacyPolicy () {
    window.open('https://brave.com/privacy#rewards', '_blank')
  }

  onDismissPromo = (promo: PromoType, event: React.MouseEvent<HTMLDivElement>) => {
    this.actions.dismissPromoPrompt(promo)
    event.preventDefault()
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
            errorText={[getLocale('redirectModalDeviceLimitReachedText')]}
            titleText={getLocale('redirectModalDeviceLimitReachedTitle')}
            learnMore={'https://support.brave.com/hc/en-us/articles/360056508071'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
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
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'kycRequiredModal':
        return (
          <ModalRedirect
            id={'redirect-modal-id-verification-required'}
            errorText={[getLocale('redirectModalKYCRequiredText').replace('$1', getWalletProviderName(externalWallet))]}
            titleText={getLocale('redirectModalKYCRequiredTitle')}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
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
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'regionNotSupportedModal':
        return (
          <ModalRedirect
            id={'redirect-modal-region-not-supported'}
            errorText={[getLocale('redirectModalRegionNotSupportedText')]}
            titleText={getLocale('redirectModalRegionNotSupportedTitle')}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'show':
        return (
          <ModalRedirect
            id={'redirect-modal-show'}
            titleText={getLocale('processingRequest')}
            walletType={walletType}
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
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdBlockedUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-blocked-user'}
            errorText={[getLocale('redirectModalUpholdBlockedUserText')]}
            titleText={getLocale('redirectModalUpholdBlockedUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdCustomerDueDiligenceRequiredModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-customer-due-diligence-required'}
            errorText={[getLocale('redirectModalUpholdCustomerDueDiligenceRequiredText')]}
            titleText={getLocale('redirectModalUpholdCustomerDueDiligenceRequiredTitle')}
            learnMore={'https://wallet.uphold.com/customer-due-diligence'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdPendingUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-pending-user'}
            errorText={[getLocale('redirectModalUpholdPendingUserText')]}
            titleText={getLocale('redirectModalUpholdPendingUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/206695986-How-do-I-sign-up-for-Uphold-Web-'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'upholdRestrictedUserModal':
        return (
          <ModalRedirect
            id={'redirect-modal-uphold-restricted-user'}
            errorText={[getLocale('redirectModalUpholdRestrictedUserText')]}
            titleText={getLocale('redirectModalUpholdRestrictedUserTitle')}
            learnMore={'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      case 'walletOwnershipVerificationFailureModal':
        return (
          <ModalRedirect
            id={'redirect-modal-wallet-ownership-verification-failure'}
            errorText={[getLocale('redirectModalWalletOwnershipVerificationFailureText').replace('$1', getWalletProviderName(externalWallet))]}
            errorTextLink={'https://community.brave.com'}
            titleText={getLocale('redirectModalWalletOwnershipVerificationFailureTitle')}
            buttonText={getLocale('redirectModalClose')}
            walletType={walletType}
            onClick={this.actions.hideRedirectModal}
          />
        )
      default:
        return null
    }
  }

  renderOnboardingPromo () {
    const { adsData, showOnboarding, ui } = this.props.rewardsData
    const { promosDismissed } = ui
    const promoKey = 'rewards-tour'

    if (showOnboarding ||
        adsData && adsData.adsEnabled ||
        promosDismissed && promosDismissed[promoKey]) {
      return null
    }

    const onTakeTour = () => {
      this.setState({ showRewardsTour: true })
    }

    const onClose = () => {
      this.actions.dismissPromoPrompt(promoKey)
      this.actions.saveOnboardingResult('dismissed')
    }

    return (
      <TourPromoWrapper>
        <RewardsTourPromo onTakeTour={onTakeTour} onClose={onClose} />
      </TourPromoWrapper>
    )
  }

  renderPromos = () => {
    const { currentCountryCode, ui } = this.props.rewardsData
    const { promosDismissed } = ui

    return (
      <>
        {this.renderOnboardingPromo()}
        {getActivePromos(this.props.rewardsData).map((key: PromoType) => {
          if (promosDismissed && promosDismissed[key]) {
            return null
          }

          const promo = getPromo(key, this.props.rewardsData) as Promo
          const { supportedLocales } = promo

          if (supportedLocales && supportedLocales.length && !supportedLocales.includes(currentCountryCode)) {
            return null
          }

          return (
            <SidebarPromo
              {...promo}
              key={`${key}-promo`}
              onDismissPromo={this.onDismissPromo.bind(this, key)}
              link={promo.link}
            />
          )
        })}
      </>
    )
  }

  renderRewardsTour () {
    if (!this.state.showRewardsTour) {
      return null
    }

    const {
      adsData,
      contributionMonthly,
      externalWallet,
      parameters
    } = this.props.rewardsData

    const onDone = () => {
      this.setState({ showRewardsTour: false, firstTimeSetup: false })
    }

    const onAdsPerHourChanged = (adsPerHour: number) => {
      this.actions.onAdsSettingSave('adsPerHour', adsPerHour)
    }

    const onAcAmountChanged = (amount: number) => {
      this.actions.onSettingSave('contributionMonthly', amount)
    }

    const onVerifyClick = () => {
      if (externalWallet && externalWallet.verifyUrl) {
        window.open(externalWallet.verifyUrl, '_self')
      }
    }

    return (
      <RewardsTourModal
        layout='wide'
        firstTimeSetup={this.state.firstTimeSetup}
        adsPerHour={adsData.adsPerHour}
        autoContributeAmount={contributionMonthly}
        autoContributeAmountOptions={parameters.autoContributeChoices}
        externalWalletProvider={externalWallet ? externalWallet.type : ''}
        onAdsPerHourChanged={onAdsPerHourChanged}
        onAutoContributeAmountChanged={onAcAmountChanged}
        onVerifyWalletClick={onVerifyClick}
        onDone={onDone}
        onClose={onDone}
      />
    )
  }

  renderSettings () {
    const { showOnboarding } = this.props.rewardsData

    if (showOnboarding) {
      const onTakeTour = () => {
        this.setState({ showRewardsTour: true })
      }

      const onEnable = () => {
        this.actions.saveOnboardingResult('opted-in')
        this.setState({ showRewardsTour: true, firstTimeSetup: true })
      }

      return (
        <SettingsOptInForm onTakeTour={onTakeTour} onEnable={onEnable} />
      )
    }

    return (
      <>
        <MainToggle
          testId={'mainToggle'}
          onTOSClick={this.openTOS}
          onPrivacyClick={this.openPrivacyPolicy}
        />
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
        <Grid columns={3} customStyle={{ gridGap: '32px', alignItems: 'stretch' }}>
          <Column size={2} customStyle={{ flexDirection: 'column' }}>
            {this.getRedirectModal()}
            {this.renderSettings()}
          </Column>
          <Column size={1} customStyle={{ flexDirection: 'column' }}>
            {this.getPromotionsClaims()}
            <PageWallet showManageWalletButton={true} />
            {this.renderPromos()}
          </Column>
        </Grid>
        {this.renderRewardsTour()}
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
