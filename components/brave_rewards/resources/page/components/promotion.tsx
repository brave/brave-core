/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  GrantWrapper,
  GrantCaptcha,
  GrantComplete
} from '../../ui/components'
import GrantClaim, { Type } from '../../ui/components/grantClaim'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import { getLocale } from '../../../../common/locale'
import GrantError from '../../ui/components/grantError'

interface State {
  promotionShow: boolean
}

interface Props extends Rewards.ComponentProps {
  promotion: Rewards.Promotion
  onlyAnonWallet?: boolean
}

// TODO add local when we know what we will get from the server
class Promotion extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      promotionShow: true
    }
  }

  get actions () {
    return this.props.actions
  }

  onShow = (promotionId?: string) => {
    this.actions.claimPromotion(promotionId)
  }

  onHide = () => {
    this.actions.resetPromotion(this.props.promotion.promotionId)
  }

  onFinish = () => {
    this.setState({
      promotionShow: false
    })
    this.actions.deletePromotion(this.props.promotion.promotionId)
  }

  onSolution = (x: number, y: number) => {
    this.actions.attestPromotion(this.props.promotion.promotionId, x, y)
  }

  getCaptcha = () => {
    const { promotion } = this.props

    if (!promotion) {
      return
    }

    if (promotion.captchaStatus === 'generalError') {
      return (
        <GrantWrapper
          onClose={this.onHide}
          title={getLocale('grantGeneralErrorTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGeneralErrorButton')}
            text={getLocale('grantGeneralErrorText')}
            onButtonClick={this.onHide}
          />
        </GrantWrapper>
      )
    }

    if (!promotion.captchaImage || !promotion.hint) {
      return
    }

    return (
      <GrantWrapper
        onClose={this.onHide}
        title={promotion.captchaStatus === 'wrongPosition' ? getLocale('notQuite') : getLocale('almostThere')}
        text={getLocale('proveHuman')}
      >
        <GrantCaptcha
          onSolution={this.onSolution}
          captchaImage={promotion.captchaImage}
          hint={promotion.hint}
          isWindows={navigator.platform === 'Win32'}
        />
      </GrantWrapper>
    )
  }

  getFinish = (type: string, tokens: string, date: string) => {
    const { onlyAnonWallet } = this.props
    const tokenString = onlyAnonWallet ? getLocale('point') : getLocale('token')

    let title = getLocale('grantFinishTitleUGP')
    let text = getLocale('grantFinishTextUGP', { currency: tokenString })
    let tokenTitle = onlyAnonWallet
      ? getLocale('grantFinishPointUGP')
      : getLocale('grantFinishTokenUGP')

    if (type === 'ads') {
      title = getLocale('grantFinishTitleAds')
      text = getLocale('grantFinishTextAds')
      tokenTitle = getLocale('grantFinishTokenAds')
    }

    return (
      <GrantWrapper
        data-test-id={'grantWrapper'}
        onClose={this.onFinish}
        title={title}
        text={text}
      >
        <GrantComplete
          onClose={this.onFinish}
          amount={tokens}
          date={date}
          testId={'newTokenGrant'}
          tokenTitle={tokenTitle}
          onlyAnonWallet={onlyAnonWallet}
        />
      </GrantWrapper>
    )
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
    let tokens = '0.0'
    let date = ''

    if (promotion.type) {
      type = this.convertPromotionTypesToType(promotion.type)
    }

    if (promotion.promotionId) {
      promoId = promotion.promotionId
    }
    if (promotion.amount) {
      tokens = promotion.amount.toFixed(1)
    }

    if (promotion.type !== 1) { // Rewards.PromotionTypes.ADS
      date = new Date(promotion.expiresAt).toLocaleDateString()
    }

    return (
      <>
        {
          this.state.promotionShow && type
            ? <GrantClaim type={type as Type} onClaim={this.onShow.bind(this, promoId)} testId={'claimGrant'}/>
            : null
        }
        {
          promotion.captchaImage && promotion.captchaStatus !== 'finished'
            ? this.getCaptcha()
            : null
        }
        {
          promotion.captchaStatus === 'finished'
            ? this.getFinish(type, tokens, date)
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
