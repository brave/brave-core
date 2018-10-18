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

type Step = '' | 'complete'

interface State {
  grantShow: boolean
  grantStep: Step
}

class GrantMobile extends React.Component<{}, State > {
  constructor (props: {}) {
    super(props)
    this.state = {
      grantShow: true,
      grantStep: ''
    }
  }

  onGrantHide = () => {
    this.setState({ grantStep: '' })
  }

  onClaim = () => {
    this.setState({ grantStep: 'complete' })
  }

  onComplete = () => {
    this.setState({ grantStep: '', grantShow: false })
  }

  render () {
    return (
      <>
        {
          this.state.grantShow
          ? <GrantClaim
            isMobile={true}
            onClaim={this.onClaim}
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
            <GrantComplete onClose={this.onComplete} amount={'30.0'} date={'8/15/2018'} />
          </GrantWrapper>
          : null
        }
      </>
    )
  }
}

export default GrantMobile
