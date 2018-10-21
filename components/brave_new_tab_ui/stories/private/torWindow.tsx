/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { OpenNewIcon } from '../../../../src/components/icons'
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
  TorLockImage,
  Separator,
  FakeButton
  // Link
} from '../../../../src/features/newTab/private'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window-tor.svg')

interface State {
  learnMoreAboutPrivateWindowsWithTor: boolean
  learnMoreAboutDuckDuckGo: boolean
  learnMoreAboutTorInBrave: boolean
}

export default class TorTab extends React.PureComponent<{}, State> {
  render () {
    return (
      <Grid>
        <HeaderBox>
          <HeaderGrid>
            <PrivateImage src={privateWindowImg} />
            <div>
              <SubTitle>{locale.headerLabel}</SubTitle>
              <Title>{locale.headerTorTitle}</Title>
              <Text>{locale.headerTorText}</Text>
              {/* <FakeButton
                href='https://support.brave.com/hc/en-us/articles/360018121491'
                target='_blank'
              >
                {locale.headerTorButton}
              </FakeButton> */}
            </div>
          </HeaderGrid>
        </HeaderBox>
        <Box style={{ minHeight: '471px' }}>
          <Content>
            <DuckDuckGoImage />
            <SubTitle>{locale.boxDdgLabel}</SubTitle>
            <Title>{locale.boxDdgTitle}</Title>
            <Text>{locale.boxDdgText2}</Text>
          </Content>
          <Separator />
          <ButtonGroup>
            <FakeButton settings={true} href='https://support.brave.com/hc/en-us/articles/360018266171' target='_blank'>
              <span>{locale.learnMore}</span>
              <OpenNewIcon />
            </FakeButton>
            {/* <Link
              href='https://support.brave.com/hc/en-us/articles/360018266171'
              target='_blank'
            >
              {locale.learnMore}
            </Link> */}
          </ButtonGroup>
        </Box>
        <Box>
          <Content>
            <TorLockImage />
            <SubTitle>{locale.boxTorLabel2}</SubTitle>
            <Title>{locale.boxTorTitle}</Title>
            <Text>{locale.boxTorText}</Text>
          </Content>
          <Separator />
          <FakeButton
            href='https://support.brave.com/hc/en-us/articles/360018121491'
            target='_blank'
          >
            {locale.boxTorButton}
          </FakeButton>
        </Box>
      </Grid>
    )
  }
}
