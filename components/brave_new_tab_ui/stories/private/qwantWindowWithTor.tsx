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
  PurpleButton
} from '../../../../src/features/newTab'

import PrivateWindowsWithTorModal from './modals/privateWindowsWithTorModal'
import TorInBraveModal from './modals/torInBraveModal'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window-tor.svg')

interface State {
  learnMoreAboutPrivateWindowsWithTor: boolean
  learnMoreAboutTorInBrave: boolean
}

export default class QwantTab extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      learnMoreAboutPrivateWindowsWithTor: false,
      learnMoreAboutTorInBrave: false
    }
  }

  onClickLearnMoreAboutPrivateWindowsWithTor = () => {
    this.setState({ learnMoreAboutPrivateWindowsWithTor: !this.state.learnMoreAboutPrivateWindowsWithTor })
  }

  onClickLearnMoreAboutTorInBrave = () => {
    this.setState({ learnMoreAboutTorInBrave: !this.state.learnMoreAboutTorInBrave })
  }

  render () {
    return (
      <>
        {
          this.state.learnMoreAboutPrivateWindowsWithTor
            ? <PrivateWindowsWithTorModal onClose={this.onClickLearnMoreAboutPrivateWindowsWithTor} />
            : null
        }
        {
          this.state.learnMoreAboutTorInBrave
            ? <TorInBraveModal onClose={this.onClickLearnMoreAboutTorInBrave} />
            : null
        }
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
              <SubTitle>{locale.boxTorLabel}</SubTitle>
              <Title>{locale.boxTorTitle}</Title>
              <Text>{locale.boxTorText}</Text>
            </Content>
            <Separator />
            <PurpleButton text={locale.boxTorButton} onClick={this.onClickLearnMoreAboutTorInBrave} />
          </Box>
        </Grid2Columns>
      </>
    )
  }
}
