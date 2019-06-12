/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton } from 'brave-ui/features/welcome'
import { SelectBox, Toggle } from 'brave-ui/features/shields'

// Images
import { WelcomeImportImage } from 'brave-ui/features/welcome/images'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

interface State {
  browserProfiles: any,
  selectedBrowserProfile: BrowserProfile | null
}

interface BrowserProfile {
  autofillFormData: boolean,
  cookies: boolean,
  favorites: boolean,
  history: boolean,
  index: number,
  ledger: boolean,
  name: string,
  passwords: boolean,
  search: boolean,
  stats: boolean,
  windows: boolean
}

export default class ImportBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      browserProfiles: undefined,
      selectedBrowserProfile: null
    }

    let self = this

    window.cr.sendWithPromise('initializeImportDialog')
      .then(function (browser_profiles: any) {
        self.setState({browserProfiles: browser_profiles})
      })
  }

  onChangeImporter = (event: React.ChangeEvent<HTMLSelectElement>) => {
    if (event.target.value === '') {
      this.setState({ selectedBrowserProfile: null })
      return
    }

    let selectedEntry = this.state.browserProfiles.find((entry: BrowserProfile) => {
      return entry.index.toString() === event.target.value
    })

    if (selectedEntry) {
      this.setState({ selectedBrowserProfile: selectedEntry })
    }
  }

  onToggleChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    console.log('todo: ...')
  }

// class ImportDataBrowserProxyImpl {
//     /** @override */
//     initializeImportDialog() {
//       return cr.sendWithPromise('initializeImportDialog');
//     }

//     /** @override */
//     importData(sourceBrowserProfileIndex) {
//       chrome.send('importData', [sourceBrowserProfileIndex]);
//     }

//     /** @override */
//     importFromBookmarksFile() {
//       chrome.send('importFromBookmarksFile');
//     }
//   }

  onHandleImport = () => {
    const { onClick } = this.props

    let sourceBrowserProfileIndex: number
    sourceBrowserProfileIndex = this.state && this.state.selectedBrowserProfile && this.state.selectedBrowserProfile.index || 0

    chrome.send('importData', [sourceBrowserProfileIndex])
    onClick()
  }

  render () {
    const { index, currentScreen } = this.props
    const { browserProfiles, selectedBrowserProfile } = this.state
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeImportImage />
        <Title>{getLocale('importFromAnotherBrowser')}</Title>

        <SelectBox
          onChange={this.onChangeImporter}
        >
          <option key={0} value=''>Import from...</option>
          {!browserProfiles ? null : browserProfiles.map((browserProfile: BrowserProfile, index: number) =>
            <option
              key={index + 1}
              value={browserProfile.index}
            >
              {browserProfile.name}
            </option>
          )}
        </SelectBox>

        {
          !selectedBrowserProfile
          ? null
          : <div>
            <h3>Import from {selectedBrowserProfile.name}</h3>
            <div>
              {!selectedBrowserProfile.cookies? null : <div><Toggle id='import_cookies' onChange={this.onToggleChanged} />Import cookies</div>}
              {!selectedBrowserProfile.favorites? null : <div><Toggle id='import_favorites' onChange={this.onToggleChanged} />Favorites</div>}
              {!selectedBrowserProfile.history? null : <div><Toggle id='import_history' onChange={this.onToggleChanged} />History</div>}
              {!selectedBrowserProfile.ledger? null : <div><Toggle id='import_ledger' onChange={this.onToggleChanged} />Ledger</div>}
              {!selectedBrowserProfile.passwords? null : <div><Toggle id='import_passwords' onChange={this.onToggleChanged} />Passwords</div>}
              {!selectedBrowserProfile.search? null : <div><Toggle id='import_search' onChange={this.onToggleChanged} />Search</div>}
              {!selectedBrowserProfile.stats? null : <div><Toggle id='import_stats' onChange={this.onToggleChanged} />Stats</div>}
              {!selectedBrowserProfile.windows? null : <div><Toggle id='import_windows' onChange={this.onToggleChanged} />Windows</div>}
            </div>
          </div>
        }

        <Paragraph>{getLocale('setupImport')}</Paragraph>
          <PrimaryButton
            level='primary'
            type='accent'
            size='large'
            text={getLocale('import')}
            onClick={this.onHandleImport}
          />
      </Content>
    )
  }
}
