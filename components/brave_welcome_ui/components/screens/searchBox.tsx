
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
}

interface State {
  searchEngineSelected: boolean,
  searchEngineIndex: number,
  isDefaultSearchGoogle: boolean,
  searchProviders: Array<SearchEngineEntry>
}

interface SearchEngineEntry {
  canBeDefault: boolean,
  canBeEdited: boolean,
  canBeRemoved: boolean,
  default: boolean,
  displayName: string,
  iconURL: string,
  id: number,
  isOmniboxExtension: boolean,
  keyword: string,
  modelIndex: number,
  name: string,
  url: string,
  urlLocked: boolean
}

export default class SearchEngineBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      searchEngineSelected: false,
      searchEngineIndex: -1,
      isDefaultSearchGoogle: true,
      searchProviders: []
    }

    // @ts-ignore
    window.cr.sendWithPromise('getSearchEnginesList').then(response => {
      const defaultEntry = response.defaults.find(function (entry: SearchEngineEntry) {
        return entry.default
      })
      this.setState({
        isDefaultSearchGoogle: defaultEntry.name === 'Google',
        searchProviders: response.defaults,
        searchEngineIndex: defaultEntry.modelIndex
      })
    })
  }

  onChangeDefaultSearchEngine = (event: React.ChangeEvent<HTMLSelectElement>) => {
    const modelIndex = parseInt(event.target.value, 10)
    chrome.send('setDefaultSearchEngine', [modelIndex])
    this.setState({ searchEngineSelected: true, searchEngineIndex: modelIndex })
  }

  render () {
    const { index, currentScreen, onClick } = this.props
    const { searchEngineSelected, searchEngineIndex, searchProviders, isDefaultSearchGoogle } = this.state
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
            <SelectBox
              value={searchEngineIndex.toString()}
              onChange={this.onChangeDefaultSearchEngine}
            >
              {searchProviders.map((provider, index) =>
                <option
                  key={index + 1}
                  value={provider.modelIndex.toString()}
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
