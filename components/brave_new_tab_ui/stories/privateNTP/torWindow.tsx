/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  Grid3Columns,
  HeaderGrid,
  HeaderBox,
  Title,
  Text,
  PrivacyEyeImage,
  PrivateImage,
  TorHelpText,
  TorStatusContainer,
  TorStatusGrid,
  TorStatusIndicator,
  TorStatusText
} from '../../components/private'

import locale from './fakeLocale'

const privateWindowImg = require('../../../img/newtab/private-window-tor.svg')
const privacyEyeImg = require('../../../img/newtab/privacy-eye.svg')
const torConnectedImg = require('../../../img/newtab/tor-connected.svg')
const torDisconnectedImg = require('../../../img/newtab/tor-disconnected.svg')

export default class TorTab extends React.PureComponent<{}, {}> {
  get torCircuitEstablished () {
    return false
  }

  get torInitProgress () {
    return 10
  }

  get torCircuitEstablishedOrInitializing () {
    return this.torCircuitEstablished || !!this.torInitProgress
  }

  get torStatus () {
    if (this.torCircuitEstablished) {
      return locale.torStatusConnected
    }
    if (this.torInitProgress) {
      return locale.torStatusInitializing + String(this.torInitProgress) + '%'
    }
    return locale.torStatusDisconnected
  }

  render () {
    return (
      <>
        <Grid3Columns>
          <HeaderBox>
            <HeaderGrid isStandalonePrivatePage={true}>
              <PrivateImage src={privateWindowImg} isStandalonePrivatePage={true} />
              <Title isStandalonePrivatePage={true}>{locale.headerTorTitle}</Title>
            </HeaderGrid>
          </HeaderBox>
          <PrivacyEyeImage src={privacyEyeImg} />
          <Text isStandalonePrivatePage={true}>{locale.headerTorText1}</Text>
          <Text isStandalonePrivatePage={true}>{locale.headerTorText2}</Text>
        </Grid3Columns>
        <TorStatusGrid>
          <TorStatusContainer isTorCircuitEstablishedOrInitializing={this.torCircuitEstablishedOrInitializing}>
            <TorStatusIndicator
              src={this.torCircuitEstablishedOrInitializing ? torConnectedImg : torDisconnectedImg}
              isTorCircuitInitializing={!!this.torInitProgress}
            />
            <TorStatusText>{this.torStatus}</TorStatusText>
          </TorStatusContainer>
        </TorStatusGrid>
        {!this.torCircuitEstablished && <TorHelpText>{this.torInitProgress ? locale.torHelpConnecting : locale.torHelpDisconnected}</TorHelpText>}
      </>
    )
  }
}
