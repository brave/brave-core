/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import Promotion from './promotion'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'
import PageWallet from './pageWallet'
import MonthlyContributionBox from './monthlyContributionBox'
import {
  MainToggleMobile,
  SettingsPageMobile,
  WalletInfoHeader
} from '../../ui/components/mobile'

import { getLocale } from 'brave-ui/helpers'
import { AlertWallet } from '../../ui/components/mobile/walletInfoHeader'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

interface State {
  walletShown: boolean
}

export interface Props extends Rewards.ComponentProps {
  rewardsEnabled?: boolean
}

class SettingsPage extends React.Component<Props, State> {
  private balanceTimerId: number

  constructor (props: Props) {
    super(props)
    this.state = {
      walletShown: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onToggleWallet = () => {
    if (this.state.walletShown) {
      window.location.hash = ''
    } else {
      window.location.hash = '#rewards-summary'
    }

    this.setState({ walletShown: !this.state.walletShown })
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
    this.actions.getExternalWallet()
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

  componentDidUpdate (prevProps: Props) {
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
      !prevProps.rewardsData.adsData.adsEnabled &&
      this.props.rewardsData.adsData.adsEnabled
    ) {
      this.actions.getStatement()
    }
  }

  startRewards () {
    if (this.props.rewardsData.firstLoad) {
      this.actions.getAdsData()
    } else {
      // normal load
      this.refreshActions()
    }

    this.actions.getRewardsParameters()
    this.actions.getContributionAmount()
    this.actions.getAutoContributeProperties()
    this.actions.getBalance()
    this.actions.getExternalWallet()
    this.balanceTimerId = setInterval(() => {
      this.actions.getBalance()
    }, 60000)

    this.actions.fetchPromotions()
    this.isWalletUrl()
    window.addEventListener('popstate', (e) => {
      this.isWalletUrl()
    })
    window.addEventListener('hashchange', (e) => {
      this.isWalletUrl()
    })
  }

  stopRewards () {
    clearInterval(this.balanceTimerId)
    this.balanceTimerId = -1
  }

  isWalletUrl = () => {
    const walletShown = (
      window &&
      window.location &&
      window.location.hash &&
      window.location.hash === '#rewards-summary'
    )

    this.setState({
      walletShown: !!walletShown
    })
  }

  walletAlerts = (): AlertWallet | null => {
    const { externalWallet } = this.props.rewardsData
    const { disconnectWalletError } = this.props.rewardsData.ui

    if (disconnectWalletError) {
      return {
        node: <><b>{getLocale('uhOh')}</b><br />{getLocale('disconnectWalletFailed').replace('$1', utils.getWalletProviderName(externalWallet))}<br /><br /><a href='https://support.brave.com/hc/en-us/articles/360062026432'>{getLocale('learnMore')}</a></>,
        type: 'error',
        onAlertClose: () => {
          this.actions.onClearAlert('disconnectWalletError')
        }
      }
    }
    return null
  }

  getPromotionsClaims = () => {
    const { promotions, ui } = this.props.rewardsData

    if (!promotions) {
      return null
    }

    return (
      <div style={{ width: '100%' }}>
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

  render () {
    const { externalWallet, parameters, balance } = this.props.rewardsData
    const { onlyAnonWallet } = this.props.rewardsData.ui
    const { total } = balance
    const convertedBalance = utils.convertBalance((total || 0), parameters.rate)

    return (
      <SettingsPageMobile>
        <MainToggleMobile />
        {this.getPromotionsClaims()}
        <WalletInfoHeader
          onClick={this.onToggleWallet}
          balance={total.toFixed(3).toString()}
          id={'mobile-wallet'}
          onlyAnonWallet={onlyAnonWallet}
          alert={this.walletAlerts()}
          wallet={externalWallet}
          converted={`${convertedBalance} USD`}
        />
        <AdsBox />
        <ContributeBox />
        <MonthlyContributionBox />
        <TipBox />
        <PageWallet
          toggleAction={this.onToggleWallet}
          visible={this.state.walletShown}
        />
      </SettingsPageMobile>
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
