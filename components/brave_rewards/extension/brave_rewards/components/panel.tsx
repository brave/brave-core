/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { WalletAddIcon, BatColorIcon } from 'brave-ui/components/icons'
import { WalletWrapper, WalletSummary, WalletSummarySlider, WalletPanel } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

interface Props extends RewardsExtension.ComponentProps {
  windowId: number
}

interface State {
  showSummary: boolean
}

export class Panel extends React.Component<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      showSummary: true
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

  render () {
    const publisher: RewardsExtension.Publisher | undefined = this.getPublisher()

    return (
      <WalletWrapper
        compact={true}
        contentPadding={false}
        gradientTop={this.gradientColor}
        tokens={30}
        converted={'15.50 USD'}
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
        grants={[
          {
            tokens: 8,
            expireDate: '7/15/2018'
          },
          {
            tokens: 10,
            expireDate: '9/10/2018'
          },
          {
            tokens: 10,
            expireDate: '10/10/2018'
          }
        ]}
      >
        <WalletSummarySlider
          id={'panel-slider'}
          onToggle={this.doNothing}
        >
          {
            publisher && publisher.publisher_key
            ? <WalletPanel
              id={'wallet-panel'}
              platform={publisher.provider as Provider}
              publisherName={publisher.name}
              publisherImg={publisher.favicon_url || `chrome://favicon/size/48@1x/${publisher.url}/`}
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
          <WalletSummary
            compact={true}
            grant={{ tokens: 10, converted: 0.25 }}
            ads={{ tokens: 10, converted: 0.25 }}
            contribute={{ tokens: 10, converted: 0.25 }}
            donation={{ tokens: 2, converted: 0.25 }}
            tips={{ tokens: 19, converted: 5.25 }}
            total={{ tokens: 1, converted: 5.25 }}
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
