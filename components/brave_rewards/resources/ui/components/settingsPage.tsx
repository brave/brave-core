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
  private balanceTimerId: number

  onToggle = () => {
    this.actions.onSettingSave('enabledMain', !this.props.rewardsData.enabledMain)
  }

  get actions () {
    return this.props.actions
  }

  refreshActions () {
    this.actions.getCurrentReport()
    this.actions.getDonationTable()
    this.actions.getContributeList()
    this.actions.getPendingContributionsTotal()
    this.actions.getReconcileStamp()
    this.actions.getConfirmationsHistory()
    this.actions.getExcludedPublishersNumber()
    this.actions.getAdsData()
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
    this.balanceTimerId = setInterval(() => {
      this.actions.getWalletProperties()
    }, 60000)

    if (this.props.rewardsData.firstLoad === false) {
      this.refreshActions()
    }
    this.actions.checkImported()
    this.actions.getGrants()

    // one time check (legacy fix)
    // more info here https://github.com/brave/brave-browser/issues/2172
    if (!this.props.rewardsData.ui.addressCheck) {
      this.actions.getAddresses()
    }

    // one time check (legacy fix)
    if (!this.props.rewardsData.ui.paymentIdCheck) {
      // https://github.com/brave/brave-browser/issues/3060
      this.actions.getAddressesForPaymentId()
      // https://github.com/brave/brave-browser/issues/3061
      this.actions.getWalletPassphrase()
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (
      !prevProps.rewardsData.enabledMain &&
      this.props.rewardsData.enabledMain &&
      this.props.rewardsData.firstLoad === false
    ) {
      this.refreshActions()
    }

    if (
      !prevProps.rewardsData.enabledContribute &&
      this.props.rewardsData.enabledContribute
    ) {
      this.actions.getContributeList()
      this.actions.getReconcileStamp()
    }
  }

  getGrantClaims = () => {
    const { grants } = this.props.rewardsData

    if (!grants) {
      return null
    }

    return (
      <>
        {grants.map((grant?: Rewards.Grant, index?: number) => {
          if (!grant || !grant.promotionId) {
            return null
          }

          return (
            <div key={`grant-${index}`}>
              <Grant grant={grant} />
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
    const { enabledMain } = this.props.rewardsData

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
              enabledMain
              ? this.getGrantClaims()
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
