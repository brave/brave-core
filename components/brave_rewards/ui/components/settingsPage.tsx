/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Column, Grid } from 'brave-ui/components'
import { MainToggle, SettingsPage as Page } from 'brave-ui/features/rewards'
import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import DonationBox from './donationsBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Grant from './grant'

interface Props extends Rewards.ComponentProps {
}

class SettingsPage extends React.Component<Props, {}> {
  onToggle = () => {
    this.actions.onSettingSave('enabledMain', !this.props.rewardsData.enabledMain)
  }

  get actions () {
    return this.props.actions
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
    this.actions.getGrant()
  }

  render () {
    const { enabledMain, grant } = this.props.rewardsData

    return (
      <Page>
        <Grid columns={3} customStyle={{ gridGap: '32px' }}>
          <Column size={2} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <MainToggle
              onToggle={this.onToggle}
              enabled={enabledMain}
              testId={'enableMain'}
            />
            <AdsBox />
            <ContributeBox />
            <DonationBox />
          </Column>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {
              enabledMain && grant && grant.promotionId
                ? <Grant/>
                : null
            }
            <PageWallet />
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
