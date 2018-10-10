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
  GrantCaptcha,
  GrantComplete
} from 'brave-ui/features/rewards'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import { getLocale } from '../../../common/locale'
import { convertProbiToFixed } from '../utils'

interface State {
  grantShow: boolean
}

interface Props extends Rewards.ComponentProps {
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

  onGrantShow = () => {
    this.actions.getGrantCaptcha()
  }

  onGrantHide = () => {
    this.actions.onResetGrant()
  }

  onSuccess = () => {
    this.setState({
      grantShow: false
    })
    this.actions.onDeleteGrant()
  }

  onSolution = (x: number, y: number) => {
    this.actions.solveGrantCaptcha(x, y)
  }

  render () {
    const { grant } = this.props.rewardsData

    if (!grant) {
      return null
    }

    let tokens = '0.0'
    if (grant.probi) {
      tokens = convertProbiToFixed(grant.probi)
    }

    return (
      <>
        {
          this.state.grantShow
            ? <GrantClaim onClaim={this.onGrantShow}/>
            : null
        }
        {
          !grant.expiryTime && grant.captcha && grant.hint
            ? <GrantWrapper
              onClose={this.onGrantHide}
              title={grant.status === 'wrongPosition' ? getLocale('notQuite') : getLocale('almostThere')}
              text={getLocale('proveHuman')}
            >
              <GrantCaptcha onSolution={this.onSolution} dropBgImage={grant.captcha} hint={grant.hint} />
            </GrantWrapper>
            : null
        }
        {
          grant.expiryTime
            ? <GrantWrapper
              onClose={this.onSuccess}
              title={'It’s your lucky day!'}
              text={'Your token grant is on its way.'}
            >
              <GrantComplete onClose={this.onSuccess} amount={tokens} date={new Date(grant.expiryTime).toLocaleDateString()} />
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
