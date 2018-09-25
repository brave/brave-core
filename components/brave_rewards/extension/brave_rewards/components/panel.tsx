/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { WalletAddIcon, BatColorIcon } from 'brave-ui/components/icons'
import { WalletWrapper, WalletSummary, WalletSummarySlider, WalletPanel } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'
import BigNumber from 'bignumber.js'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'
import * as utils from '../../../ui/utils'

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

  componentDidMount () {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const newKey = publisher && publisher.publisher_key

    if (newKey) {
      this.setState({
        showSummary: false,
        publisherKey: newKey
      })
    }

    this.props.actions.getWalletProperties()
    this.props.actions.getCurrentReport()
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

  getPublisher = () => {
    const windowId = this.props.windowId

    if (!windowId) {
      return undefined
    }

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
        tokens: new BigNumber(grant.probi.toString()).dividedBy('1e18').toNumber(),
        expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString()
      }
    })
  }

  getWalletSummary = () => {
    const { walletProperties, report } = this.props.rewardsPanelData
    const { rates } = walletProperties
    const contributionMonthly = 10 // TODO NZ fix with new reports refactor https://github.com/brave/brave-core/pull/409
    const convertedMonthly = utils.convertBalance(contributionMonthly, rates)
    let total = contributionMonthly * -1

    let props = {
      contribute: {
        tokens: contributionMonthly,
        converted: convertedMonthly
      },
      total: {
        tokens: contributionMonthly,
        converted: convertedMonthly
      }
    }

    if (report) {
      if (report.ads) {
        props['ads'] = {
          tokens: report.ads,
          converted: utils.convertBalance(report.ads, rates)
        }

        total += report.ads
      }

      if (report.donations) {
        props['donation'] = {
          tokens: report.donations,
          converted: utils.convertBalance(report.donations, rates)
        }

        total -= report.donations
      }

      if (report.grants) {
        props['grant'] = {
          tokens: report.grants,
          converted: utils.convertBalance(report.grants, rates)
        }

        total += report.grants
      }

      if (report.oneTime) {
        props['tips'] = {
          tokens: report.oneTime,
          converted: utils.convertBalance(report.oneTime, rates)
        }

        total -= report.oneTime
      }

      props['total'] = {
        tokens: total,
        converted: utils.convertBalance(total, rates)
      }
    }

    return props
  }

  render () {
    const { balance, rates, grants } = this.props.rewardsPanelData.walletProperties
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()
    const converted = utils.convertBalance(balance, rates)

    return (
      <WalletWrapper
        compact={true}
        contentPadding={false}
        gradientTop={this.gradientColor}
        tokens={balance}
        converted={utils.formatConverted(converted)}
        actions={[
          {
            name: 'Add funds',
            action: this.doNothing,
            icon: <WalletAddIcon />
          },
          {
            name: 'Rewards Settings',
            action: this.doNothing,
            icon: <BatColorIcon />
          }
        ]}
        showCopy={false}
        showSecActions={false}
        connectedWallet={false}
        grants={this.getGrants(grants)}
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
              publisherImg={publisher.favicon_url || `chrome://favicon/size/48@1x/${publisher.url}`}
              monthlyAmount={10}
              isVerified={publisher.verified}
              tipsEnabled={true}
              includeInAuto={publisher.excluded}
              attentionScore={(publisher.percentage || 0).toString()}
              donationAmounts={
                [5, 10, 15, 20, 30, 50, 100]
              }
              onToggleTips={this.doNothing}
              donationAction={this.doNothing}
              onAmountChange={this.doNothing}
              onIncludeInAuto={this.doNothing}
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
