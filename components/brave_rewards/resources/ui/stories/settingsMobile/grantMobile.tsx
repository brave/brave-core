/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import {
  GrantClaim,
  GrantComplete,
  GrantWrapper
} from '../../../../src/features/rewards'
import { Type } from '../../../../src/features/rewards/grantClaim'

type Step = '' | 'complete'

interface State {
  grantShow: boolean
  grantStep: Step
  loading: boolean
}

interface Props {
  type: Type
  amount?: string
}

class GrantMobile extends React.Component<Props, State > {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantShow: true,
      grantStep: '',
      loading: false
    }
  }

  onGrantHide = () => {
    this.setState({ grantStep: '' })
  }

  onClaim = () => {
    this.setState({ loading: true })
    setTimeout(() => {
      this.setState({ grantStep: 'complete' })
    }, 2000)
  }

  onComplete = () => {
    this.setState({
      grantStep: '',
      grantShow: false,
      loading: false
    })
  }

  render () {
    const { type, amount } = this.props

    return (
      <>
        {
          this.state.grantShow
          ? <GrantClaim
            isMobile={true}
            type={type}
            amount={amount}
            onClaim={this.onClaim}
            loading={this.state.loading}
          />
          : null
        }
        {
          this.state.grantStep === 'complete'
          ? <GrantWrapper
            fullScreen={true}
            onClose={this.onGrantHide}
            title={'It’s your lucky day!'}
            text={'Your token grant is on its way.'}
          >
            <GrantComplete
              isMobile={true}
              onClose={this.onComplete}
              amount={'30.0'}
              date={'8/15/2018'}
            />
          </GrantWrapper>
          : null
        }
      </>
    )
  }
}

export default GrantMobile
