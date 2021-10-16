/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Grid3Columns,
  HeaderGrid,
  ControlBox,
  HeaderBox,
  ControlText,
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
import { getLocale } from '../../../common/locale'

// API
import { toggleAlternativePrivateSearchEngine } from '../../api/privateTabData'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window.svg')
const privacyEyeImg = require('../../../img/newtab/privacy-eye.svg')

interface Props {
  actions: any
  newTabData: NewTab.State
}

export default class PrivateTab extends React.PureComponent<Props, {}> {
  get useAlternativePrivateSearchEngine () {
    return (
      this.props.newTabData &&
      this.props.newTabData.useAlternativePrivateSearchEngine
    )
  }

  get showAlternativePrivateSearchEngineToggle () {
    return (
      this.props.newTabData &&
      this.props.newTabData.showAlternativePrivateSearchEngineToggle
    )
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
              <Title isStandalonePrivatePage={true}>{getLocale('headerTitle')}</Title>
            </HeaderGrid>
          </HeaderBox>
          <PrivacyEyeImage src={privacyEyeImg} />
          <Text isStandalonePrivatePage={true}>{getLocale('headerText1')}</Text>
          <Text isStandalonePrivatePage={true}>{getLocale('headerText2')} <Link href='https://support.brave.com/hc/en-us/articles/360017840332' target='_blank' isStandalonePrivatePage={true}>{getLocale('headerButton')}</Link></Text>
        </Grid3Columns>
        {
          this.showAlternativePrivateSearchEngineToggle ?
          <ControlBox>
            <ToggleGroup>
              <IconText>
                <DuckDuckGoImage isStandalonePrivatePage={true} />
                <ControlText>{getLocale('boxDdgButton')}</ControlText>
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
