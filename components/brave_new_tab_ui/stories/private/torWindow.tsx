/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
// import { OpenNewIcon } from '../../../../src/components/icons'
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
  PurpleButton,
  // FakeButton,
  Link
} from '../../../../src/features/newTab'

import PrivateWindowsWithTorModal from './modals/privateWindowsWithTorModal'
import DuckDuckGoModal from './modals/duckDuckGoModal'
import TorInBraveModal from './modals/torInBraveModal'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window-tor.svg')

interface State {
  learnMoreAboutPrivateWindowsWithTor: boolean
  learnMoreAboutDuckDuckGo: boolean
  learnMoreAboutTorInBrave: boolean
}

export default class TorTab extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      learnMoreAboutPrivateWindowsWithTor: false,
      learnMoreAboutDuckDuckGo: false,
      learnMoreAboutTorInBrave: false
    }
  }

  onClickLearnMoreAboutPrivateWindowsWithTor = () => {
    this.setState({ learnMoreAboutPrivateWindowsWithTor: !this.state.learnMoreAboutPrivateWindowsWithTor })
  }

  onClickLearnMoreAboutDuckDuckGo = () => {
    this.setState({ learnMoreAboutDuckDuckGo: !this.state.learnMoreAboutDuckDuckGo })
  }

  onClickLearnMoreAboutTorInBrave = () => {
    this.setState({ learnMoreAboutTorInBrave: !this.state.learnMoreAboutTorInBrave })
  }

  onClickLearnMore = () => {
    window.open('chrome://settings', '_blank')
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
          this.state.learnMoreAboutDuckDuckGo
            ? <DuckDuckGoModal onClose={this.onClickLearnMoreAboutDuckDuckGo} />
            : null
        }
        {
          this.state.learnMoreAboutTorInBrave
            ? <TorInBraveModal onClose={this.onClickLearnMoreAboutTorInBrave} />
            : null
        }
        <Grid>
          <HeaderBox>
            <HeaderGrid>
              <PrivateImage src={privateWindowImg} />
              <div>
                <SubTitle>{locale.headerLabel}</SubTitle>
                <Title>{locale.headerTorTitle}</Title>
                <Text>{locale.headerTorText}</Text>
                <PurpleButton text={locale.headerTorButton} onClick={this.onClickLearnMoreAboutPrivateWindowsWithTor} />
              </div>
            </HeaderGrid>
          </HeaderBox>
          <Box>
            <Content>
              <DuckDuckGoImage />
              <SubTitle>{locale.boxDdgLabel}</SubTitle>
              <Title>{locale.boxDdgTitle}</Title>
              <Text>{locale.boxDdgText2}</Text>
            </Content>
            <Separator />
            <ButtonGroup>
              {/* <FakeButton settings={true} href='chrome://settings' target='_blank'>
                <span>{locale.searchSettings}</span>
                <OpenNewIcon />
              </FakeButton> */}
              <Link onClick={this.onClickLearnMoreAboutDuckDuckGo}>{locale.learnMore}</Link>
            </ButtonGroup>
          </Box>
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
        </Grid>
      </>
    )
  }
}
