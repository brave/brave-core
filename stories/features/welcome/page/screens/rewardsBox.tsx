/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, PaymentsImage, Paragraph } from '../../../../../src/features/welcome/'

// Shared components
import { Button } from '../../../../../src/components'

// Utils
import locale from '../fakeLocale'

// Images
const paymentsImage = require('../../../../assets/img/welcome_rewards.svg')

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

export default class PaymentsBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick } = this.props
    console.log('index', index)
    console.log('current', currentScreen)
    return (
      <Content zIndex={index} active={index === currentScreen} isPrevious={index > currentScreen}>
        <PaymentsImage src={paymentsImage} />
        <Title>{locale.enableBraveRewards}</Title>
        <Paragraph>{locale.setupBraveRewards}</Paragraph>
        <Button
          level='primary'
          type='accent'
          size='large'
          text={locale.getStarted}
          onClick={onClick}
        />
      </Content>
    )
  }
}
