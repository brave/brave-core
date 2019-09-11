/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Grid,
  HeaderGrid,
  ButtonGroup,
  Box,
  Content,
  HeaderBox,
  Title,
  SubTitle,
  Text,
  PrivateImage,
  DuckDuckGoImage,
  Separator,
  FakeButton,
  Link
} from '../../components/private'

// Components
import { Toggle } from '../../components/toggle'
import TorContent from './torContent'

// Helpers
import { getLocale } from '../../../common/locale'

// API
import { toggleAlternativePrivateSearchEngine } from '../../api/privateTabData'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window.svg')

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

  onChangePrivateSearchEngine = () => {
    toggleAlternativePrivateSearchEngine()
  }

  render () {
    return (
      <Grid>
        <HeaderBox>
          <HeaderGrid>
            <PrivateImage src={privateWindowImg} />
            <div>
              <SubTitle>{getLocale('headerLabel')}</SubTitle>
              <Title>{getLocale('headerTitle')}</Title>
              <Text>{getLocale('headerText')} <Link href='https://support.brave.com/hc/en-us/articles/360017840332' target='_blank'>{getLocale('headerButton')}</Link>
              </Text>
            </div>
          </HeaderGrid>
        </HeaderBox>
        <Box style={{ minHeight: '475px' }}>
          <Content>
            <DuckDuckGoImage />
            <SubTitle>{getLocale('boxDdgLabel')}</SubTitle>
            <Title>{getLocale('boxDdgTitle')}</Title>
            <Text>{getLocale('boxDdgText')}</Text>
          </Content>
          <Separator />
          <ButtonGroup>
            <FakeButton withToggle={true}>
              <span style={{ color: '#fff' }}>{getLocale('boxDdgButton')}</span>
              <Toggle
                id='duckduckgo'
                checked={this.useAlternativePrivateSearchEngine}
                onChange={this.onChangePrivateSearchEngine}
              />
            </FakeButton>
            <Link href='https://support.brave.com/hc/en-us/articles/360018266171' target='_blank'>{getLocale('learnMore')}</Link>
          </ButtonGroup>
        </Box>
        <Box>
          <TorContent />
          <Separator />
          <FakeButton href='https://support.brave.com/hc/en-us/articles/360018121491' target='_blank'>
            {getLocale('boxTorButton')}
          </FakeButton>
        </Box>
      </Grid>
    )
  }
}
