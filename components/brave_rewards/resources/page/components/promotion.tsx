/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import GrantClaim, { Type } from '../../ui/components/grantClaim'

// Utils
import * as rewardsActions from '../actions/rewards_actions'

interface Props extends Rewards.ComponentProps {
  promotion: Rewards.Promotion
}

// TODO add local when we know what we will get from the server
class Promotion extends React.Component<Props> {
  get actions () {
    return this.props.actions
  }

  onShow = (promotionId?: string) => {
    this.actions.claimPromotion(promotionId)
  }

  convertPromotionTypesToType = (type: Rewards.PromotionTypes) => {
    switch (type) {
      case 1: { // Rewards.PromotionTypes.ADS
        return 'ads'
      }
      default: {
        return 'ugp'
      }
    }
  }

  render () {
    const { promotion } = this.props

    if (!promotion) {
      return null
    }

    let type = 'ugp'
    let promoId

    if (promotion.type) {
      type = this.convertPromotionTypesToType(promotion.type)
    }

    if (promotion.promotionId) {
      promoId = promotion.promotionId
    }

    return (
      <>
        {
          type
            ? <GrantClaim type={type as Type} onClaim={this.onShow.bind(this, promoId)} testId={'claimGrant'}/>
            : null
        }
      </>
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
)(Promotion)
