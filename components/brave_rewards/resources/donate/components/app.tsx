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

  generateDonationOverlay = (publisher: RewardsDonate.Publisher | undefined) => {
    setTimeout(() => {
      this.onClose()
    }, 3000)

    let verified = false
    let domain = ''
    let logo = ''
    if (publisher) {
      if (publisher.provider && publisher.name) {
        domain = publisher.name
      } else {
        domain = publisher.publisherKey
      }

      verified = publisher.verified
      logo = publisher.logo
    }

    const internalFavicon = /^https:\/\/[a-z0-9-]+\.invalid(\/)?$/
    if (internalFavicon.test(logo)) {
      logo = `chrome://favicon/size/160@2x/${logo}`
    }

    if (!verified) {
      logo = ''
    }

    return (
      <DonationOverlay
        onClose={this.onClose}
        success={true}
        domain={domain}
        logo={logo}
      />
    )
  }

  render () {
    const { finished, error, publisher } = this.props.rewardsDonateData
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
