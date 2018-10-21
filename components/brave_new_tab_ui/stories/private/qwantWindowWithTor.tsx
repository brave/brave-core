/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  Grid2Columns,
  Box,
  Content,
  HeaderBox,
  Title,
  SubTitle,
  Text,
  TorImage,
  TorLockImage,
  Separator,
  FakeButton
} from '../../../../src/features/newTab/private'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window-tor.svg')

export default class QwantTab extends React.PureComponent<{}, {}> {
  render () {
    return (
      <Grid2Columns>
        <HeaderBox>
          <div>
            <TorImage src={privateWindowImg} />
            <div>
              <SubTitle>{locale.headerLabel}</SubTitle>
              <Title>{locale.headerTorTitle}</Title>
              <Text>{locale.headerTorText}</Text>
            </div>
          </div>
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
      </Grid2Columns>
    )
  }
}
