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
import { StyledDisabledContent, StyledHeading, StyledText } from './style'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { getLocale } from '../../../../common/locale'

interface State {
  mainToggle: boolean
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
      mainToggle: true,
      walletShown: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onToggle = () => {
    this.setState({ mainToggle: !this.state.mainToggle })
    this.actions.onSettingSave('enabledMain', !this.props.rewardsData.enabledMain)
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
    this.actions.getContributeList()
    this.actions.getTransactionHistory()
    this.actions.getAdsData()
    this.actions.getExcludedSites()
  }

  componentDidMount () {
    if (this.props.rewardsData.firstLoad === null) {
      // First load ever
      this.actions.onSettingSave('firstLoad', true)
      this.actions.getWalletPassphrase()
    } else if (this.props.rewardsData.firstLoad) {
      // Second load ever
      this.actions.onSettingSave('firstLoad', false)
    }

    this.actions.getWalletProperties()
    this.actions.getBalance()
    this.balanceTimerId = setInterval(() => {
      this.actions.getBalance()
    }, 60000)

    if (this.props.rewardsData.firstLoad === false) {
      this.refreshActions()
    } else {
      this.actions.getAdsData()
    }

    this.isWalletUrl()

    window.addEventListener('popstate', (e) => {
      this.isWalletUrl()
    })
    window.addEventListener('hashchange', (e) => {
      this.isWalletUrl()
    })
  }

  componentDidUpdate (prevProps: Props) {
    if (
      !prevProps.rewardsData.enabledMain &&
      this.props.rewardsData.enabledMain
    ) {
      this.refreshActions()
    }

    if (
      !prevProps.rewardsData.adsData.adsEnabled &&
      this.props.rewardsData.adsData.adsEnabled
    ) {
      this.actions.getTransactionHistory()
    }

    if (
      !prevProps.rewardsData.enabledContribute &&
      this.props.rewardsData.enabledContribute
    ) {
      this.actions.getContributeList()
    }
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

  getPromotionsClaim = () => {
    const { promotions, ui } = this.props.rewardsData

    if (!promotions) {
      return null
    }

    return (
      <>
        {promotions.map((promotion?: Rewards.Promotion, index?: number) => {
          if (!promotion || !promotion.promotionId) {
            return null
          }

          return (
            <div key={`grant-${index}`}>
              <Promotion promotion={promotion} onlyAnonWallet={ui.onlyAnonWallet} />
            </div>
          )
        })}
      </>
    )
  }

  componentWillUnmount () {
    clearInterval(this.balanceTimerId)
  }

  render () {
    const { enabledMain, balance } = this.props.rewardsData
    const { onlyAnonWallet } = this.props.rewardsData.ui
    const { total } = balance
    const convertedBalance = utils.convertBalance((total || 0), balance.rates)

    return (
      <SettingsPageMobile>
        <MainToggleMobile
          onToggle={this.onToggle}
          enabled={enabledMain}
        />
        {
          !this.state.mainToggle && !enabledMain
          ? <StyledDisabledContent>
              <StyledHeading>
                {getLocale('rewardsWhy')}
              </StyledHeading>
              <StyledText>
                {getLocale('whyBraveRewardsDesc1')}
              </StyledText>
              <StyledText>
                {getLocale('whyBraveRewardsDesc2')}
              </StyledText>
            </StyledDisabledContent>
          : null
        }
        {
          enabledMain
          ? this.getPromotionsClaim()
          : null
        }
        <WalletInfoHeader
          onClick={this.onToggleWallet}
          balance={total.toFixed(1).toString()}
          id={'mobile-wallet'}
          onlyAnonWallet={onlyAnonWallet}
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
