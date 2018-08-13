/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  GrantClaim,
  GrantWrapper,
  GrantInit,
  GrantCaptcha,
  GrantComplete
} from 'brave-ui/features/rewards'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import { getLocale } from '../../../common/locale'

interface State {
  grantShow: boolean
  showWelcome: boolean
}

interface Props extends Rewards.ComponentProps {
}

// TODO add local when we know what we will get from the server
class Grant extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantShow: true,
      showWelcome: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onGrantShow = () => {
    this.setState({
      showWelcome: true
    })
  }

  onGrantHide = () => {
    this.setState({
      showWelcome: false
    })
    this.actions.onResetPromotion()
  }

  onSuccess = () => {
    this.setState({
      grantShow: false,
      showWelcome: false
    })
    this.actions.onDeletePromotion()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solvePromotionCaptcha(x, y)
  }

  onAccept = () => {
    this.actions.getPromotionCaptcha()
  }

  onDeny = () => {
    // TODO should we delay next fetch?
    this.actions.onDeletePromotion()
  }

  render () {
    const { promotion } = this.props.rewardsData

    if (!promotion) {
      return null
    }

    return (
      <>
        {
          this.state.grantShow
            ? <GrantClaim onClick={this.onGrantShow}/>
            : null
        }
        {
          promotion.expireDate
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'It’s your lucky day!'}
              text={'Your token grant is on its way.'}
            >
              <GrantComplete onClose={this.onSuccess} amount={promotion.amount} date={new Date(promotion.expireDate).toLocaleDateString()} />
            </GrantWrapper>
            : null
        }
        {
          !promotion.expireDate && promotion.captcha
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={promotion.error === 'wrongPosition' ? getLocale('notQuite') : getLocale('almostThere')}
              text={getLocale('proveHuman')}
            >
              <GrantCaptcha onSolution={this.onSolution} dropBgImage={promotion.captcha} />
            </GrantWrapper>
            : null
        }
        {
          !promotion.expireDate && !promotion.captcha && this.state.showWelcome
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'Good news!'}
              text={`Free ${promotion.amount} BAT have been awarded to you so you can support more publishers.`}
            >
              <GrantInit onAccept={this.onAccept} onDeny={this.onDeny} />
            </GrantWrapper>
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
