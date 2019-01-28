/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import Banner from './siteBanner'
import DonationOverlay from 'brave-ui/features/rewards/donationOverlay'

// Utils
import * as rewardsActions from '../actions/donate_actions'

interface Props extends RewardsDonate.ComponentProps {
}

export class App extends React.Component<Props, {}> {

  get actions () {
    return this.props.actions
  }

  onClose = () => {
    this.actions.onCloseDialog()
  }

  generateDonationOverlay = (publisher: RewardsDonate.Publisher) => {
    let domain = ''
    let monthlyDate
    let recurringDonation
    const {
      currentTipAmount,
      currentTipRecurring,
      recurringDonations
    } = this.props.rewardsDonateData

    const publisherKey = publisher && publisher.publisherKey

    if (!publisherKey ||
        (currentTipRecurring && !recurringDonations)) {
      return null
    }

    if (recurringDonations) {
      recurringDonation = recurringDonations.find((donation: RewardsDonate.RecurringDonation) => {
        return donation.publisherKey === publisherKey
      })
    }

    if (recurringDonation && recurringDonation.monthlyDate) {
      monthlyDate = new Date(recurringDonation.monthlyDate * 1000).toLocaleDateString()
    }

    if (publisher.provider && publisher.name) {
      domain = publisher.name
    } else {
      domain = publisherKey
    }

    setTimeout(() => {
      this.onClose()
    }, 3000)

    return (
      <DonationOverlay
        onClose={this.onClose}
        success={true}
        domain={domain}
        amount={currentTipAmount}
        monthlyDate={monthlyDate}
        logo={publisher && publisher.logo}
      />
    )
  }

  render () {
    const { finished, error, publisher } = this.props.rewardsDonateData

    if (!publisher) {
      return null
    }

    return (
      <>
        {
          !finished && !error
          ? <Banner />
          : null
        }
        {
          finished
          ? this.generateDonationOverlay(publisher)
          : null
        }
      </>
    )
  }
}

export const mapStateToProps = (state: RewardsDonate.ApplicationState) => ({
  rewardsDonateData: state.rewardsDonateData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(App)
