
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Content,
  Title,
  Paragraph,
  SelectGrid,
  PrimaryButton,
  SelectBox
} from '../../../components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeSearchImage } from '../../../components/images'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
  fakeOnChange: () => void
  isDefaultSearchGoogle: boolean
}

interface State {
  searchEngineSelected: boolean
}

export default class SearchEngineBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      searchEngineSelected: false
    }
  }

  onChangeDefaultSearchEngine = (event: React.ChangeEvent<HTMLSelectElement>) => {
    console.log((event.target.value) !== '')
    this.setState({ searchEngineSelected: (event.target.value) !== '' })
    this.props.fakeOnChange()
  }

  render () {
    const { index, currentScreen, onClick, isDefaultSearchGoogle } = this.props
    const { searchEngineSelected } = this.state
    const bodyText = isDefaultSearchGoogle ? `${locale.chooseSearchEngine} ${locale.privateExperience}` : locale.chooseSearchEngine
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeSearchImage />
        <Title>{locale.setDefaultSearchEngine}</Title>
        <Paragraph>{bodyText}</Paragraph>
          <SelectGrid>
            <SelectBox onChange={this.onChangeDefaultSearchEngine}>
              <option value=''>{locale.selectSearchEngine}</option>
              <option value='DuckDuckGo'>{locale.fakeSearchProvider1}</option>
              <option value='Google'>{locale.fakeSearchProvider2}</option>
            </SelectBox>
            <PrimaryButton
              level='primary'
              type='accent'
              size='large'
              text={locale.setDefault}
              disabled={!searchEngineSelected}
              onClick={onClick}
            />
          </SelectGrid>
      </Content>
    )
  }
}
