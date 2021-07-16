
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SelectGrid, SelectBox } from '../../components'

// Images
import { WelcomeSearchImage } from '../../components/images'

// Utils
import { getLocale } from '../../../common/locale'

export interface Props {
  index: number
  currentScreen: number
  onClick: () => void
  changeDefaultSearchProvider: (searchProvider: string) => void
  searchProviders: Array<Welcome.SearchEngineEntry>
}

interface State {
  searchEngineSelected: boolean,
  isDefaultSearchGoogle: boolean,
}

export default class SearchEngineBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      searchEngineSelected: false,
      isDefaultSearchGoogle: true
    }
  }

  onChangeDefaultSearchEngine = (event: React.ChangeEvent<HTMLSelectElement>) => {
    if (event.target.value === '') {
      this.setState({ searchEngineSelected: false })
      return
    }
    this.props.changeDefaultSearchProvider(event.target.value)
    this.setState({ searchEngineSelected: true })
  }

  getDefaultSearchProvider = (searchEngineEntries: Array<Welcome.SearchEngineEntry>): Welcome.SearchEngineEntry => {
    const defaultSearchProvider = searchEngineEntries
      .filter((searchEngine: Welcome.SearchEngineEntry) => searchEngine.default)
    return defaultSearchProvider[0]
  }

  getProviderDisplayName = (searchProvider: Welcome.SearchEngineEntry, defaultSearchProvider: Welcome.SearchEngineEntry): string =>
    searchProvider.name === defaultSearchProvider.name
    ? `${searchProvider.name} (${getLocale('default')})`
    : searchProvider.name

  render () {
    const { index, currentScreen, onClick, searchProviders } = this.props
    const { searchEngineSelected } = this.state
    const defaultProvider = this.getDefaultSearchProvider(searchProviders)
    const bodyText = defaultProvider
      ? getLocale('chooseSearchEngine') || ''
      : ''

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
            <SelectBox
              onChange={this.onChangeDefaultSearchEngine}
            >
              <option key={0} value=''>{getLocale('selectSearchEngine')}</option>
              {
                (searchProviders && Array.isArray(searchProviders) && searchProviders.length > 0)
                ? searchProviders.map((provider, index) =>
                  <option
                    key={index + 1}
                    value={provider.modelIndex.toString()}
                  >
                    {this.getProviderDisplayName(provider, defaultProvider)}
                  </option>
                )
                : null
              }
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
