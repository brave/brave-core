/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
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
  DuckDuckGoImage, TorLockImage,
  Separator,
  FakeButton,
  Link

} from '../../../../src/features/newTab/private'
import { Toggle } from '../../../../src/features/shields'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window.svg')

export default class PrivateTab extends React.PureComponent<{}, {}> {
  render () {
    return (
      <Grid>
        <HeaderBox>
          <HeaderGrid>
            <PrivateImage src={privateWindowImg} />
            <div>
              <SubTitle>{locale.headerLabel}</SubTitle>
              <Title>{locale.headerTitle}</Title>
              <Text>{locale.headerText} <Link href='https://support.brave.com/hc/en-us/articles/360017840332' target='_blank'>{locale.headerButton}</Link></Text>
              {/* <FakeButton href='https://support.brave.com/hc/en-us/articles/360017840332' target='_blank'>
                {locale.headerButton}
              </FakeButton> */}
            </div>
          </HeaderGrid>
        </HeaderBox>
        <Box style={{ minHeight: '475px' }}>
          <Content>
            <DuckDuckGoImage />
            <SubTitle>{locale.boxDdgLabel}</SubTitle>
            <Title>{locale.boxDdgTitle}</Title>
            <Text>{locale.boxDdgText}</Text>
          </Content>
          <Separator />
          <ButtonGroup>
            <FakeButton>
              <span>{locale.boxDdgButton}</span>
              <Toggle />
            </FakeButton>
            <Link href='https://support.brave.com/hc/en-us/articles/360018266171' target='_blank'>{locale.learnMore}</Link>
          </ButtonGroup>
        </Box>
        <Box>
          <Content>
            <TorLockImage />
            <SubTitle>{locale.boxTorLabel}</SubTitle>
            <Title>{locale.boxTorTitle}</Title>
            <Text>{locale.boxTorText2}</Text>
          </Content>
          <Separator />
          <FakeButton href='https://support.brave.com/hc/en-us/articles/360018121491' target='_blank'>
            {locale.boxTorButton}
          </FakeButton>
        </Box>
      </Grid>
    )
  }
}
