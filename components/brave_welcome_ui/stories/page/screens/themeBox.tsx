/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, SelectGrid, SelectBox } from '../../../components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeThemeImage } from '../../../components/images'

interface Props {
  index: number
  currentScreen: number
}

interface State {
  themeSelected: boolean
}

export default class ThemingBox extends React.PureComponent<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      themeSelected: false
    }
  }

  onChangeThemeOption = (event: React.ChangeEvent<HTMLSelectElement>) => {
    this.setState({ themeSelected: event.target.value !== '' })
  }

  render () {
    const { index, currentScreen } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeThemeImage />
        <Title>{locale.chooseYourTheme}</Title>
        <Paragraph>{locale.findToolbarTheme}</Paragraph>
        <SelectGrid>
            <SelectBox onChange={this.onChangeThemeOption}>
              <option value=''>{locale.selectTheme}</option>
              <option value='Light'>{locale.themeOption1}</option>
              <option value='Dark'>{locale.themeOption2}</option>
              <option value='System'>{locale.themeOption3}</option>
            </SelectBox>
        </SelectGrid>
      </Content>
    )
  }
}
