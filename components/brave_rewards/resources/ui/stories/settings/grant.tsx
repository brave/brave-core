/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import {
  GrantCaptcha,
  GrantClaim,
  GrantComplete,
  GrantInit,
  GrantWrapper
} from '../../../../src/features/rewards'

// Assets
const captchaDrop = require('../../../assets/img/captchaDrop.png')

type Step = '' | 'init' | 'captcha' | 'complete'

interface State {
  grantShow: boolean
  grantStep: Step
}

class Grant extends React.Component<{}, State > {
  constructor (props: {}) {
    super(props)
    this.state = {
      grantShow: true,
      grantStep: ''
    }
  }

  onGrantShow = () => {
    this.setState({ grantShow: false, grantStep: 'init' })
  }

  onGrantHide = () => {
    this.setState({ grantStep: '' })
  }

  onGrantStep = (step: Step) => {
    this.setState({ grantStep: step })
  }

  render () {
    return (
      <>
        {
          this.state.grantShow
          ? <GrantClaim onClick={this.onGrantShow}/>
          : null
        }
        {
          this.state.grantStep === 'init'
          ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'Good news!'}
              text={'Free 30 BAT have been awarded to you so you can support more publishers.'}
          >
            <GrantInit onAccept={this.onGrantStep.bind(this, 'captcha')} onLater={this.onGrantHide} />
          </GrantWrapper>
          : null
        }
        {
          this.state.grantStep === 'captcha'
          ? <GrantWrapper
              onClose={this.onGrantHide}
              title={'Almost there…'}
              text={'Prove that you are human!'}
          >
            <GrantCaptcha onSolution={this.onGrantStep.bind(this, 'complete')} dropBgImage={captchaDrop} />
          </GrantWrapper>
          : null
        }
        {
          this.state.grantStep === 'complete'
          ? <GrantWrapper
            onClose={this.onGrantHide}
            title={'It’s your lucky day!'}
            text={'Your token grant is on its way.'}
          >
            <GrantComplete onClose={this.onGrantHide} amount={30} date={'8/15/2018'} />
          </GrantWrapper>
          : null
        }
      </>
    )
  }
}

export default Grant
