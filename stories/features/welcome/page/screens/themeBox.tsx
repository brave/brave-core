/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, ThemeImage, Paragraph } from '../../../../../src/features/welcome/'

// Shared components
import { Button } from '../../../../../src/components'

// Utils
import locale from '../fakeLocale'

// Images
const themeImage = require('../../../../assets/img/welcome_theme.svg')

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

export default class ThemingBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick } = this.props
    return (
      <Content zIndex={index} active={index === currentScreen} isPrevious={index > currentScreen}>
        <ThemeImage src={themeImage} />
        <Title>{locale.chooseYourTheme}</Title>
        <Paragraph>{locale.findToolbarTheme}</Paragraph>
          <Button
            level='primary'
            type='accent'
            size='large'
            text={locale.theme}
            onClick={onClick}
          />
      </Content>
    )
  }
}
