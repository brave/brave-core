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
} from 'brave-ui/features/newTab'

// Components
import { Toggle } from 'brave-ui/features/shields'
import TorContent from './torContent'

// Helpers
import { getLocale } from '../../../common/locale'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window.svg')

interface Props {
  useAlternativePrivateSearchEngine: boolean
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

export default class PrivateTab extends React.PureComponent<Props, {}> {
  render () {
    const { useAlternativePrivateSearchEngine, onChangePrivateSearchEngine } = this.props
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
                checked={useAlternativePrivateSearchEngine}
                onChange={onChangePrivateSearchEngine}
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
