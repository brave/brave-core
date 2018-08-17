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
import BigNumber from 'bignumber.js'

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
    this.actions.onResetGrant()
  }

  onSuccess = () => {
    this.setState({
      grantShow: false,
      showWelcome: false
    })
    this.actions.onDeleteGrant()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solveGrantCaptcha(x, y)
  }

  onAccept = () => {
    this.actions.getGrantCaptcha()
  }

  onDeny = () => {
    // TODO should we delay next fetch?
    this.actions.onDeleteGrant()
  }

  render () {
    const { grant } = this.props.rewardsData

    if (!grant) {
      return null
    }

    let tokens = 0
    if (grant.probi) {
      tokens = new BigNumber(grant.probi.toString()).dividedBy('1e18').toNumber()
    }

    return (
      <>
        {
          this.state.grantShow
            ? <GrantClaim onClick={this.onGrantShow}/>
            : null
        }
        {
          grant.expiryTime
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'It’s your lucky day!'}
              text={'Your token grant is on its way.'}
            >
              <GrantComplete onClose={this.onSuccess} amount={tokens} date={new Date(grant.expiryTime).toLocaleDateString()} />
            </GrantWrapper>
            : null
        }
        {
          !grant.expiryTime && grant.captcha
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={grant.status === 'wrongPosition' ? getLocale('notQuite') : getLocale('almostThere')}
              text={getLocale('proveHuman')}
            >
              <GrantCaptcha onSolution={this.onSolution} dropBgImage={grant.captcha} />
            </GrantWrapper>
            : null
        }
        {
          !grant.expiryTime && !grant.captcha && this.state.showWelcome
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'Good news!'}
              text={`Free ${grant.probi} BAT have been awarded to you so you can support more publishers.`}
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
