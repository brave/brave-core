/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Grid3Columns,
  HeaderGrid,
  HeaderBox,
  Link,
  PrivacyEyeImage,
  PrivateImage,
  Text,
  Title,
  TorHelpText,
  TorStatusContainer,
  TorStatusGrid,
  TorStatusIndicator,
  TorStatusText
} from '../../components/private'

// Helpers
import { getLocale, getLocaleWithTag } from '../../../common/locale'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window-tor.svg')
const privacyEyeImg = require('../../../img/newtab/privacy-eye.svg')
const torConnectedImg = require('../../../img/newtab/tor-connected.svg')
const torDisconnectedImg = require('../../../img/newtab/tor-disconnected.svg')

interface Props {
  actions: any
  newTabData: NewTab.State
}

export default class TorTab extends React.PureComponent<Props, {}> {
  get torCircuitEstablishedOrInitializing () {
    const { newTabData } = this.props
    return newTabData && (newTabData.torCircuitEstablished || !!newTabData.torInitProgress)
  }

  get torStatus () {
    if (this.props.newTabData &&
        this.props.newTabData.torCircuitEstablished) {
      return getLocale('torStatusConnected')
    }
    if (this.props.newTabData &&
        this.props.newTabData.torInitProgress) {
      return getLocale('torStatusInitializing',
        { percentage: String(this.props.newTabData.torInitProgress) })
    }
    return getLocale('torStatusDisconnected')
  }

  renderHelpText (isConnecting: boolean) {
    const { beforeTag, duringTag, afterTag } = getLocaleWithTag(isConnecting ? 'torHelpConnecting' : 'torHelpDisconnected')
    if (this.props.newTabData && !this.props.newTabData.torCircuitEstablished) {
      return (
        <Text>
          {beforeTag}
            <Link
              href='chrome://settings/extensions'
              style={{ margin: 0 }}
            >
            {duringTag}
            </Link>
          {afterTag}
          {!isConnecting && <Link href='https://support.brave.com/' target='_blank' style={{ margin: '0 0 0 5px' }}>{getLocale('torHelpContactSupport')}</Link>}
        </Text>
      )
    }
    return ''
  }

  render () {
    const { newTabData } = this.props
    return (
      <>
        <Grid3Columns>
          <HeaderBox>
            <HeaderGrid isStandalonePrivatePage={true}>
              <PrivateImage src={privateWindowImg} isStandalonePrivatePage={true} />
              <Title isStandalonePrivatePage={true}>{getLocale('headerTorTitle')}</Title>
            </HeaderGrid>
          </HeaderBox>
          <PrivacyEyeImage src={privacyEyeImg} />
          <Text isStandalonePrivatePage={true}>{getLocale('headerTorText1')}</Text>
          <Text isStandalonePrivatePage={true}>{getLocale('headerTorText2')}</Text>
        </Grid3Columns>
        <TorStatusGrid>
          <TorStatusContainer isTorCircuitEstablishedOrInitializing={this.torCircuitEstablishedOrInitializing}>
            <TorStatusIndicator
              src={this.torCircuitEstablishedOrInitializing ? torConnectedImg : torDisconnectedImg}
              isTorCircuitInitializing={newTabData && !!newTabData.torInitProgress}
            />
            <TorStatusText>{this.torStatus}</TorStatusText>
          </TorStatusContainer>
        </TorStatusGrid>
        {
          newTabData && !newTabData.torCircuitEstablished &&
          <TorHelpText>
            {this.renderHelpText(!!newTabData.torInitProgress)}
          </TorHelpText>
        }
      </>
    )
  }
}
