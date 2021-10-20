/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Grid3Columns,
  HeaderGrid,
  ControlBox,
  ControlText,
  HeaderBox,
  IconText,
  Title,
  Text,
  ToggleGroup,
  PrivacyEyeImage,
  PrivateImage,
  DuckDuckGoImage,
  Link
} from '../../components/private'

// Components
import { Toggle } from '../../components/toggle'

// Helpers
import locale from './fakeLocale'

// API
import { toggleAlternativePrivateSearchEngine } from '../../api/privateTabData'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window.svg')
const privacyEyeImg = require('../../../img/newtab/privacy-eye.svg')

export default class PrivateTab extends React.PureComponent<{}, {}> {
  get useAlternativePrivateSearchEngine () {
    return false
  }

  get showAlternativePrivateSearchEngineToggle () {
    return false
  }

  onChangePrivateSearchEngine = () => {
    toggleAlternativePrivateSearchEngine()
  }

  render () {
    return (
      <>
        <Grid3Columns>
          <HeaderBox>
            <HeaderGrid isStandalonePrivatePage={true}>
              <PrivateImage src={privateWindowImg} isStandalonePrivatePage={true} />
              <Title isStandalonePrivatePage={true}>{locale.headerTitle}</Title>
            </HeaderGrid>
          </HeaderBox>
          <PrivacyEyeImage src={privacyEyeImg} />
          <Text isStandalonePrivatePage={true}>{locale.headerText1}</Text>
          <Text isStandalonePrivatePage={true}>{locale.headerText2} <Link href='https://support.brave.com/hc/en-us/articles/360017840332' target='_blank' isStandalonePrivatePage={true}>{locale.headerButton}</Link></Text>
        </Grid3Columns>
        {
          this.showAlternativePrivateSearchEngineToggle ?
          <ControlBox>
            <ToggleGroup>
              <IconText>
                <DuckDuckGoImage />
                <ControlText>{locale.boxDdgButton}</ControlText>
              </IconText>
              <Toggle
                id='duckduckgo'
                size='large'
                checked={this.useAlternativePrivateSearchEngine}
                onChange={this.onChangePrivateSearchEngine}
              />
            </ToggleGroup>
          </ControlBox>
          : null
        }
      </>
    )
  }
}
