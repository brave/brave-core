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
  PurpleButton,
  FakeButton
  // Link
} from '../../../../src/features/newTab'
import { Toggle } from '../../../../src/features/shields'

import PrivateWindowsModal from './modals/privateWindowsModal'
import DuckDuckGoModal from './modals/duckDuckGoModal'
import TorInBraveModal from './modals/torInBraveModal'

import locale from './fakeLocale'
const privateWindowImg = require('../../../assets/img/private-window.svg')

interface State {
  learnMoreAboutPrivateWindows: boolean
  learnMoreAboutDuckDuckGo: boolean
  learnMoreAboutTorInBrave: boolean
}

export default class PrivateTab extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      learnMoreAboutPrivateWindows: false,
      learnMoreAboutDuckDuckGo: false,
      learnMoreAboutTorInBrave: false
    }
  }

  onClickLearnMoreAboutPrivateWindows = () => {
    this.setState({ learnMoreAboutPrivateWindows: !this.state.learnMoreAboutPrivateWindows })
  }

  onClickLearnMoreAboutDuckDuckGo = () => {
    this.setState({ learnMoreAboutDuckDuckGo: !this.state.learnMoreAboutDuckDuckGo })
  }

  onClickLearnMoreAboutTorInBrave = () => {
    this.setState({ learnMoreAboutTorInBrave: !this.state.learnMoreAboutTorInBrave })
  }

  render () {
    console.log(this.state.learnMoreAboutPrivateWindows)
    return (
      <>
        {
          this.state.learnMoreAboutPrivateWindows
            ? <PrivateWindowsModal onClose={this.onClickLearnMoreAboutPrivateWindows} />
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
                <Title>{locale.headerTitle}</Title>
                <Text>{locale.headerText}</Text>
                <PurpleButton
                  onClick={this.onClickLearnMoreAboutPrivateWindows}
                  text={locale.headerButton}
                />
              </div>
            </HeaderGrid>
          </HeaderBox>
          <Box>
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
              {/* <Link onClick={this.onClickLearnMoreAboutDuckDuckGo}>{locale.learnMore}</Link> */}
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
            <PurpleButton
              onClick={this.onClickLearnMoreAboutTorInBrave}
              text={locale.boxTorButton}
            />
          </Box>
        </Grid>
      </>
    )
  }
}
