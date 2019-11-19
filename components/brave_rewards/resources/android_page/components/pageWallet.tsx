/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { AlertWallet } from '../../ui/components/walletWrapper'
import {
  WalletSummary,
  WalletWrapper,
  WalletEmpty,
  WalletOff
} from '../../ui/components'
import { CloseStrokeIcon } from 'brave-ui/components/icons'
import { StyledWalletClose, StyledWalletOverlay, StyledWalletWrapper } from './style'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

interface State {
  activeTabId: number
}

interface Props extends Rewards.ComponentProps {
  visible?: boolean
  toggleAction: () => void
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)

    this.state = {
      activeTabId: 0
    }
  }

  get actions () {
    return this.props.actions
  }

  getConversion = () => {
    const balance = this.props.rewardsData.balance
    return utils.convertBalance(balance.total.toString(), balance.rates)
  }

  generatePromotions = () => {
    const promotions = this.props.rewardsData.promotions
    if (!promotions) {
      return []
    }

    let claimedPromotions = promotions.filter((promotion: Rewards.Promotion) => {
      return promotion.status === 4 // PromotionStatus::FINISHED
    })

    return claimedPromotions.map((promotion: Rewards.Promotion) => {
      return {
        amount: promotion.amount,
        expiresAt: new Date(promotion.expiresAt * 1000).toLocaleDateString(),
        type: promotion.type || 0
      }
    })
  }

  walletAlerts = (): AlertWallet | null => {
    const { walletServerProblem } = this.props.rewardsData.ui

    if (walletServerProblem) {
      return {
        node: <React.Fragment><b>{getLocale('uhOh')}</b> {getLocale('serverNotResponding')}</React.Fragment>,
        type: 'error'
      }
    }

    return null
  }

  getWalletSummary = () => {
    const { balance, reports } = this.props.rewardsData

    let props = {}

    const currentTime = new Date()
    const reportKey = `${currentTime.getFullYear()}_${currentTime.getMonth() + 1}`
    const report: Rewards.Report = reports[reportKey]
    if (report) {
      for (let key in report) {
        const item = report[key]

        if (item.length > 1 && key !== 'total') {
          const tokens = utils.convertProbiToFixed(item)
          props[key] = {
            tokens,
            converted: utils.convertBalance(tokens, balance.rates)
          }
        }
      }
    }

    return {
      report: props
    }
  }

  render () {
    const { visible, toggleAction } = this.props
    const {
      enabledMain,
      balance,
      ui,
      pendingContributionTotal
    } = this.props.rewardsData
    const { emptyWallet, onlyAnonWallet } = ui
    const { total } = balance
    const pendingTotal = parseFloat((pendingContributionTotal || 0).toFixed(1))

    let showCopy = false
    if (!onlyAnonWallet) {
      showCopy = true
    }

    if (!visible) {
      return null
    }

    return (
      <React.Fragment>
        <StyledWalletOverlay>
          <StyledWalletClose>
            <CloseStrokeIcon onClick={toggleAction}/>
          </StyledWalletClose>
          <StyledWalletWrapper>
            <WalletWrapper
              balance={total.toFixed(1)}
              converted={utils.formatConverted(this.getConversion())}
              actions={[]}
              compact={true}
              isMobile={true}
              showCopy={showCopy}
              showSecActions={false}
              grants={this.generatePromotions()}
              alert={this.walletAlerts()}
              onlyAnonWallet={onlyAnonWallet}
            >
              {
                enabledMain
                ? emptyWallet
                  ? <WalletEmpty hideAddFundsText={true} />
                  : <WalletSummary
                    reservedAmount={pendingTotal}
                    onlyAnonWallet={onlyAnonWallet}
                    reservedMoreLink={'https://brave.com/faq-rewards/#unclaimed-funds'}
                    {...this.getWalletSummary()}
                  />
                : <WalletOff/>
              }
            </WalletWrapper>
          </StyledWalletWrapper>
        </StyledWalletOverlay>
      </React.Fragment>
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
)(PageWallet)
