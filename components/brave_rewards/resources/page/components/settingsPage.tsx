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
import MonthlyContributionBox from './monthlyContributionBox'
import QRBox from './qrBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Promotion from './promotion'
import { getLocale } from '../../../../common/locale'
import { getActivePromos, getPromo, PromoType, Promo } from '../promos'

interface Props extends Rewards.ComponentProps {
}

interface State {
  redirectModalDisplayed: 'hide' | 'show'
}

class SettingsPage extends React.Component<Props, State> {
  private balanceTimerId: number

  constructor (props: Props) {
    super(props)
    this.state = {
      redirectModalDisplayed: 'hide'
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
    if (this.props.rewardsData.firstLoad === null) {
      // First load ever
      this.actions.onSettingSave('firstLoad', true, false)
    } else if (this.props.rewardsData.firstLoad) {
      // Second load ever
      this.actions.onSettingSave('firstLoad', false, false)
    }

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
      this.actions.getExternalWallet('uphold')
    }
  }

  stopRewards () {
    clearInterval(this.balanceTimerId)
    this.balanceTimerId = -1
  }

  startRewards () {
    if (this.props.rewardsData.firstLoad) {
      this.actions.getBalanceReport(new Date().getMonth() + 1, new Date().getFullYear())
      this.actions.getAdsData()
      this.actions.getPendingContributions()
      this.actions.getCountryCode()
    } else {
      // normal load
      this.refreshActions()
    }

    this.actions.getRewardsParameters()
    this.actions.getContributionAmount()
    this.actions.getAutoContributeProperties()
    this.actions.getBalance()
    this.balanceTimerId = setInterval(() => {
      this.actions.getBalance()
    }, 60000)

    this.actions.fetchPromotions()
    this.actions.getExternalWallet('uphold')
    this.actions.getOnboardingStatus()

    if (window.location.pathname.length > 1) {
      const pathElements = window.location.pathname.split('/')
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
    const { promotions, ui } = this.props.rewardsData

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
              <Promotion promotion={promotion} onlyAnonWallet={ui.onlyAnonWallet} />
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
    const { ui } = this.props.rewardsData

    if (ui.modalRedirect === 'show') {
      return (
        <ModalRedirect
          id={'redirect-modal-show'}
          titleText={getLocale('processingRequest')}
        />
      )
    }

    if (ui.modalRedirect === 'notAllowed') {
      return (
        <ModalRedirect
          id={'redirect-modal-not-allowed'}
          errorText={getLocale('redirectModalNotAllowed')}
          titleText={getLocale('redirectModalErrorWallet')}
          buttonText={getLocale('redirectModalClose')}
          onClick={this.actions.hideRedirectModal}
        />
      )
    }

    if (ui.modalRedirect === 'batLimit') {
      return (
        <ModalRedirect
          id={'redirect-modal-bat-limit'}
          titleText={getLocale('redirectModalBatLimitTitle')}
          errorText={getLocale('redirectModalBatLimitText')}
          buttonText={getLocale('redirectModalClose')}
          onClick={this.actions.hideRedirectModal}
        />
      )
    }

    if (ui.modalRedirect === 'error') {
      return (
        <ModalRedirect
          id={'redirect-modal-error'}
          errorText={getLocale('redirectModalError')}
          buttonText={getLocale('processingRequestButton')}
          titleText={getLocale('processingRequest')}
          onClick={this.onRedirectError}
        />
      )
    }

    return null
  }

  renderPromos = () => {
    const { currentCountryCode, ui } = this.props.rewardsData
    const { promosDismissed } = ui

    return (
      <>
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

  render () {
    return (
      <Page>
        <Grid columns={3} customStyle={{ gridGap: '32px' }}>
          <Column size={2} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {this.getRedirectModal()}
            <MainToggle
              testId={'mainToggle'}
              onTOSClick={this.openTOS}
              onPrivacyClick={this.openPrivacyPolicy}
            />
            <QRBox />
            <AdsBox />
            <ContributeBox />
            <MonthlyContributionBox />
            <TipBox />
          </Column>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {this.getPromotionsClaims()}
            <PageWallet />
            {this.renderPromos()}
          </Column>
        </Grid>
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
