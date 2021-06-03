/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  GrantClaim,
  GrantComplete,
  GrantWrapper
} from '../../ui/components'
import { Type as PromotionType } from '../../ui/components/grantClaim'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

type Step = '' | 'complete'

interface State {
  promotionShow: boolean
  promotionStep: Step
  loading: boolean
}

interface Props extends Rewards.ComponentProps {
  promotion: Rewards.Promotion
}

type GrantWrapperContentProps = {
  title: string
  text: string
}

type GrantCompleteContentProps = {
  amountTitleText: string
  dateTitleText: string
}

function getGrantWrapperContentProps (grantType: PromotionType): GrantWrapperContentProps {
  if (grantType === 'ads') {
    return {
      title: getLocale('grantTitleAds'),
      text: getLocale('grantSubtitleAds')
    }
  }
  return {
    title: getLocale('grantTitleUGP'),
    text: getLocale('grantSubtitleUGP')
  }
}

function getGrantCompleteContentProps (grantType: PromotionType): GrantCompleteContentProps {
  if (grantType === 'ads') {
    return {
      amountTitleText: getLocale('grantAmountTitleAds'),
      dateTitleText: getLocale('grantDateTitleAds')
    }
  }
  return {
    amountTitleText: getLocale('grantAmountTitleUGP'),
    dateTitleText: getLocale('grantDateTitleUGP')
  }
}

// TODO add local when we know what we will get from the server
class Promotion extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      promotionShow: true,
      promotionStep: '',
      loading: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onClaim = (promotionId?: string) => {
    this.setState({ loading: true })
    this.actions.claimPromotion(promotionId)
    this.setState({ promotionStep: 'complete' })
  }

  onSuccess = (promotionId: string) => {
    this.setState({
      promotionShow: false,
      promotionStep: '',
      loading: false
    })
    this.actions.deletePromotion(promotionId)
  }

  validPromotion = (tokens?: string) => {
    const { promotion } = this.props
    const { safetyNetFailed } = this.props.rewardsData

    if (!tokens || !promotion) {
      return false
    }

    if (safetyNetFailed) {
      this.setState({ loading: false })
      return false
    }

    // Promotion types other than Rewards.PromotionTypes.ADS must have
    // a valid expiration
    if (!promotion.expiresAt && promotion.type !== 1) {
      return false
    }

    return tokens !== '0.000'
  }

  render () {
    const { promotion } = this.props

    if (!promotion) {
      return null
    }

    // Handle that ugp type string can actually be 'android' on android
    let type: PromotionType = promotion.type === 1 ? 'ads' : 'ugp'
    let promoId
    let tokens = '0.000'
    let date = ''

    if (promotion.promotionId) {
      promoId = promotion.promotionId
    }

    if (promotion.amount) {
      tokens = promotion.amount.toFixed(3)
    }

    if (promotion.type !== 1) { // Rewards.PromotionTypes.ADS
      date = new Date(promotion.expiresAt).toLocaleDateString()
    }

    // Guard against null promotion statuses
    const validPromotion = this.validPromotion(tokens)

    return (
      <React.Fragment>
        {
          (this.state.promotionShow && type)
          ? <GrantClaim
            type={type}
            isMobile={true}
            onClaim={this.onClaim.bind(this, promoId)}
            loading={this.state.loading}
          />
          : null
        }
        {
          this.state.promotionStep === 'complete' && validPromotion
            ? <GrantWrapper
              fullScreen={true}
              onClose={this.onSuccess.bind(this, promoId)}
              {...getGrantWrapperContentProps(type)}
            >
              <GrantComplete
                isMobile={true}
                onClose={this.onSuccess.bind(this, promoId)}
                amount={tokens}
                date={date}
                {...getGrantCompleteContentProps(type)}
              />
            </GrantWrapper>
            : null
        }
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
)(Promotion)
