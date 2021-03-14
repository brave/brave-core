/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { text, select } from '@storybook/addon-knobs'

// Components
import GrantClaim from '../components/grantClaim'
import GrantError from '../components/grantError'
import GrantWrapper from '../components/grantWrapper'
import GrantCaptcha from '../components/grantCaptcha'
import GrantComplete from '../components/grantComplete'

const captchaDrop = require('./img/captchaDrop.png')

const dummyClick = () => {
  console.log(dummyClick)
}

export default {
  title: 'Rewards/Grant',
  parameters: {
    layout: 'centered'
  }
}

export const _GrantClaim = () => {
  return (
    <GrantClaim
      type={select<any>('Type', { ugp: 'ugp', ads: 'ads' }, 'ugp')}
      onClaim={dummyClick}
    />
  )
}

_GrantClaim.story = {
  name: 'Grant claim'
}

export const _GrantWrapper = () => {
  return (
    <div style={{ width: '373px', height: '715px', position: 'relative' }}>
      <GrantWrapper
        onClose={dummyClick}
        title={text('Title', 'Good news!')}
        text={text(
          'Text',
          'Free 30 BAT have been awarded to you so you can support more publishers.'
        )}
      >
        Content here
      </GrantWrapper>
    </div>
  )
}

_GrantWrapper.story = {
  name: 'Grant wrapper'
}

export const _GrantCaptcha = () => {
  return (
    <div style={{ width: '373px', height: '715px', position: 'relative' }}>
      <GrantCaptcha onSolution={dummyClick} captchaImage={captchaDrop} hint={'blue'} />
    </div>
  )
}

_GrantCaptcha.story = {
  name: 'Grant captcha'
}

export const _GrantComplete = () => {
  return (
    <div style={{ width: '373px', height: '715px', position: 'relative' }}>
      <GrantComplete
        onClose={dummyClick}
        amount={'30.0'}
        date={text('Date', '8/15/2018')}
        tokenTitle={'Free token grant'}
      />
    </div>
  )
}

_GrantComplete.story = {
  name: 'Grant complete'
}

export const _GrantError = () => {
  return (
    <div style={{ width: '373px', height: '250px', position: 'relative', background: '#fff' }}>
      <GrantError
        onButtonClick={dummyClick}
        buttonText={'ok'}
        text={'The period for claiming this grant has ended'}
      />
    </div>
  )
}
