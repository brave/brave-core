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
import { convertProbiToFixed } from '../utils'
import GrantError from '../../ui/components/grantError'

interface State {
  grantShow: boolean
}

interface Props extends Rewards.ComponentProps {
  grant: Rewards.Grant
  onlyAnonWallet?: boolean
}

// TODO add local when we know what we will get from the server
class Grant extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantShow: true
    }
  }

  get actions () {
    return this.props.actions
  }

  onGrantShow = (promotionId?: string) => {
    this.actions.getGrantCaptcha(promotionId)
  }

  onGrantHide = () => {
    this.actions.onResetGrant()
  }

  onFinish = () => {
    this.setState({
      grantShow: false
    })
    this.actions.onDeleteGrant()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solveGrantCaptcha(x, y)
  }

  grantCaptcha = () => {
    const { grant } = this.props

    if (!grant) {
      return
    }

    if (grant.status === 'grantGone') {
      return (
        <GrantWrapper
          onClose={this.onFinish}
          title={getLocale('grantGoneTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGoneButton')}
            text={getLocale('grantGoneText')}
            onButtonClick={this.onFinish}
          />
        </GrantWrapper>
      )
    }

    if (grant.status === 'grantAlreadyClaimed') {
      return (
        <GrantWrapper
          onClose={this.onFinish}
          title={getLocale('grantGoneTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGoneButton')}
            text={getLocale('grantAlreadyClaimedText')}
            onButtonClick={this.onFinish}
          />
        </GrantWrapper>
      )
    }

    if (grant.status === 'generalError') {
      return (
        <GrantWrapper
          onClose={this.onGrantHide}
          title={getLocale('grantGeneralErrorTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGeneralErrorButton')}
            text={getLocale('grantGeneralErrorText')}
            onButtonClick={this.onGrantHide}
          />
        </GrantWrapper>
      )
    }

    if (!grant.captcha || !grant.hint) {
      return
    }

    return (
      <GrantWrapper
        onClose={this.onGrantHide}
        title={grant.status === 'wrongPosition' ? getLocale('notQuite') : getLocale('almostThere')}
        text={getLocale('proveHuman')}
      >
        <GrantCaptcha
          onSolution={this.onSolution}
          dropBgImage={grant.captcha}
          hint={grant.hint}
          isWindows={navigator.platform === 'Win32'}
        />
      </GrantWrapper>
    )
  }

  grantFinish = (type: string, tokens: string, date: string) => {
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

  render () {
    const { grant } = this.props

    if (!grant) {
      return null
    }

    let type = 'ugp'
    let promoId
    let tokens = '0.0'
    let date = ''

    if (grant.type) {
      type = grant.type
    }
    if (grant.promotionId) {
      promoId = grant.promotionId
    }
    if (grant.probi) {
      tokens = convertProbiToFixed(grant.probi)
    }

    if (grant.type !== 'ads') {
      date = new Date(grant.expiryTime).toLocaleDateString()
    }

    return (
      <>
        {
          this.state.grantShow && type && grant.showClaim !== false
            ? <GrantClaim type={type as Type} onClaim={this.onGrantShow.bind(this, promoId)} testId={'claimGrant'}/>
            : null
        }
        {
          !grant.expiryTime
            ? this.grantCaptcha()
            : null
        }
        {
          grant.expiryTime
            ? this.grantFinish(type, tokens, date)
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
)(Grant)
