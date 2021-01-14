/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SelectGrid, SelectBox } from '../../components'

// Images
import { WelcomeImportImage } from '../../components/images'

// Utils
import { getLocale } from '../../../common/locale'
import {
  getSelectedBrowserProfile,
  getSourceBrowserProfileIndex,
  isValidBrowserProfiles
} from '../../welcomeUtils'

export interface Props {
  index: number
  currentScreen: number
  browserProfiles: Array<Welcome.BrowserProfile>
  onClick: (sourceBrowserProfileIndex: number) => void
}

export interface State {
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

    const selectedProfile = getSelectedBrowserProfile(event.target.value, this.props.browserProfiles)
    selectedProfile && this.setState({ selectedBrowserProfile: selectedProfile })

  }

  onHandleImport = () => {
    const { onClick } = this.props
    onClick(getSourceBrowserProfileIndex(this.state))
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
              <option key={0} value=''>{getLocale('importFrom')}</option>
              {
                isValidBrowserProfiles(browserProfiles)
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
