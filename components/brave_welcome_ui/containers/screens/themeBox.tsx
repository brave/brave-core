/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SelectGrid, SelectBox } from '../../components'

// Images
import { WelcomeThemeImage } from '../../components/images'

// Utils
import { getLocale } from '../../../common/locale'

export interface Props {
  index: number
  currentScreen: number
  onClick: () => void
  onChangeTheme: (theme: string) => void
  browserThemes: Array<Welcome.BrowserTheme>
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

  onChangeTheme = (event: React.ChangeEvent<HTMLSelectElement>) => {
    if (event.target.value === '') {
      this.setState({ themeSelected: false })
      return
    }
    this.props.onChangeTheme(event.target.value)
    this.setState({ themeSelected: true })
  }

  render () {
    const { index, currentScreen, onClick, browserThemes } = this.props
    const { themeSelected } = this.state
    // Light, Dark, System
    const showSystemThemeOption = (browserThemes.length === 3)
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeThemeImage />
        <Title>{getLocale('findToolbarTheme')}</Title>
        <Paragraph>{getLocale('chooseTheme')}</Paragraph>
        <SelectGrid>
            <SelectBox
              onChange={this.onChangeTheme}
            >
              <option value=''>{getLocale('selectTheme')}</option>
              <option value='Light'>{getLocale('light')}</option>
              <option value='Dark'>{getLocale('dark')}</option>
              {showSystemThemeOption && <option value='System'>{getLocale('systemTheme')}</option>}
            </SelectBox>
          <PrimaryButton
            level='primary'
            type='accent'
            size='large'
            text={getLocale('confirmTheme')}
            disabled={!themeSelected}
            onClick={onClick}
          />
        </SelectGrid>
      </Content>
    )
  }
}
