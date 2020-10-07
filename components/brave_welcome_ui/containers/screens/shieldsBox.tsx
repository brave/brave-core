/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph } from '../../components'

// Utils
import { getLocale } from '../../../common/locale'

// Images
import { WelcomeShieldsImage } from '../../components/images'

interface Props {
  index: number
  currentScreen: number
}

export default class ShieldsBox extends React.PureComponent<Props> {
  render () {
    const text = getLocale('privacyDesc').split('$1')

    const { index, currentScreen } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeShieldsImage />
        <Title>{getLocale('privacyTitle')}</Title>
        <Paragraph>
          {text[0]}
          <a
            href='https://brave.com/p3a'
            target='_blank'
            rel='noopener noreferrer'
          >
            {text[1]}
          </a>
          {text[2]}
          <a
            href='chrome://settings/privacy'
            target='_blank'
            rel='noopener noreferrer'
          >
            {text[3]}
          </a>
          {text[4]}
        </Paragraph>
      </Content>
    )
  }
}
