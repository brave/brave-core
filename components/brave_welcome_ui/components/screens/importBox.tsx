/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SelectGrid, SelectBox } from 'brave-ui/features/welcome'

// Images
import { WelcomeImportImage } from 'brave-ui/features/welcome/images'

// Utils
import { getLocale } from '../../../common/locale'

export interface Props {
  index: number
  currentScreen: number
  browserProfiles: Array<Welcome.BrowserProfile>
  onClick: (sourceBrowserProfileIndex: number) => void
}

interface State {
  selectedBrowserProfile: Welcome.BrowserProfile | null
}

export default class ImportBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      selectedBrowserProfile: null
    }
  }

  onChangeImportSource = (event: React.ChangeEvent<HTMLSelectElement>) => {
    if (event.target.value === '') {
      this.setState({ selectedBrowserProfile: null })
      return
    }

    let selectedEntry = this.props.browserProfiles.find((entry: Welcome.BrowserProfile) => {
      return entry.index.toString() === event.target.value
    })

    if (selectedEntry) {
      this.setState({ selectedBrowserProfile: selectedEntry })
    }
  }

  onHandleImport = () => {
    const { onClick } = this.props
    let sourceBrowserProfileIndex: number
    sourceBrowserProfileIndex = this.state && this.state.selectedBrowserProfile && this.state.selectedBrowserProfile.index || 0
    onClick(sourceBrowserProfileIndex)
  }

  render () {
    const { index, currentScreen, browserProfiles } = this.props
    const { selectedBrowserProfile } = this.state
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeImportImage />
        <Title>{getLocale('importFromAnotherBrowser')}</Title>
        <Paragraph>{getLocale('setupImport')}</Paragraph>
          <SelectGrid>
            <SelectBox
              onChange={this.onChangeImportSource}
            >
              <option key={0} value=''>Import from...</option>
              {
                (browserProfiles && Array.isArray(browserProfiles) && browserProfiles.length > 0)
                ? browserProfiles.map((browserProfile, index) =>
                  <option
                    key={index + 1}
                    value={browserProfile.index}
                  >
                    {browserProfile.name}
                  </option>
                )
                : null
              }
            </SelectBox>
            <PrimaryButton
              level='primary'
              type='accent'
              size='large'
              text={getLocale('import')}
              disabled={!selectedBrowserProfile}
              onClick={this.onHandleImport}
            />
          </SelectGrid>
      </Content>
    )
  }
}
