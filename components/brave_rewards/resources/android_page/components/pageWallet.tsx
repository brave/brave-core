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
  WalletEmpty
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
    const { balance, parameters } = this.props.rewardsData
    return utils.convertBalance(balance.total, parameters.rate)
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
    const { balanceReport, parameters } = this.props.rewardsData

    let props = {}

    if (balanceReport) {
      for (let key in balanceReport) {
        const item = balanceReport[key]

        if (item !== 0) {
          const tokens = item.toFixed(3)
          props[key] = {
            tokens,
            converted: utils.convertBalance(item, parameters.rate)
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
      balance,
      ui,
      pendingContributionTotal
    } = this.props.rewardsData
    const { emptyWallet } = ui
    const { total } = balance
    const pendingTotal = parseFloat((pendingContributionTotal || 0).toFixed(3))

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
              balance={total.toFixed(3)}
              converted={utils.formatConverted(this.getConversion())}
              actions={[]}
              compact={true}
              isMobile={true}
              showCopy={true}
              showSecActions={false}
              alert={this.walletAlerts()}
              walletProvider={''}
            >
              {
                emptyWallet
                ? <WalletEmpty hideAddFundsText={true} />
                : <WalletSummary
                  reservedAmount={pendingTotal}
                  reservedMoreLink={'https://brave.com/faq-rewards/#unclaimed-funds'}
                  {...this.getWalletSummary()}
                />
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
