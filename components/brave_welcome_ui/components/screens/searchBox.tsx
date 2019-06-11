
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SelectGrid } from 'brave-ui/features/welcome'
import { SelectBox } from 'brave-ui/features/shields'

// Images
import { WelcomeSearchImage } from 'brave-ui/features/welcome/images'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
  onChange: (event: React.ChangeEvent<HTMLSelectElement>) => void
  isDefaultSearchGoogle: boolean
  // TODO Pass in search options as an array of data and define specific type definition
  searchProviders: Array<any>
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
    this.setState({ searchEngineSelected: ((event.target.value) !== '') })
    this.props.onChange(event)
  }

  render () {
    const { index, currentScreen, onClick, isDefaultSearchGoogle, searchProviders } = this.props
    const { searchEngineSelected } = this.state
    const bodyText = isDefaultSearchGoogle ? `${getLocale('chooseSearchEngine')} ${getLocale('privateExperience')}` : getLocale('chooseSearchEngine')
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeSearchImage />
        <Title>{getLocale('setDefaultSearchEngine')}</Title>
        <Paragraph>{bodyText}</Paragraph>
          <SelectGrid>
            <SelectBox onChange={this.onChangeDefaultSearchEngine}>
              <option key={0} value=''>{getLocale('selectSearchEngine')}</option>
              {searchProviders.map((provider, index) =>
                <option
                  key={index + 1}
                  value={provider.value}
                >
                  {provider.name}
                </option>
              )}
            </SelectBox>
            <PrimaryButton
              level='primary'
              type='accent'
              size='large'
              text={getLocale('setDefault')}
              disabled={!searchEngineSelected}
              onClick={onClick}
            />
          </SelectGrid>
      </Content>
    )
  }
}
