/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  Grid,
  HeaderGrid,
  Box,
  Content,
  HeaderBox,
  Title,
  SubTitle,
  Text,
  PrivateImage,
  TorLockImage,
  Separator,
  FakeButton
  // Link
} from '../../components/private'

import locale from './fakeLocale'
const privateWindowImg = require('../../../img/newtab/private-window-tor.svg')

interface State {
  learnMoreAboutPrivateWindowsWithTor: boolean
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
