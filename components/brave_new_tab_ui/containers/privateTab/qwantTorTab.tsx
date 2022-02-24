/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
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
} from '../../components/private'

// Helpers
import { getLocale } from '../../../common/locale'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window-tor.svg')

interface Props {
  actions: any
  newTabData: NewTab.State
}

export default class QwantTorTab extends React.PureComponent<Props, {}> {
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

  render () {
    return (
      <Grid2Columns>
        <HeaderBox>
          <div>
            <TorImage src={privateWindowImg} />
            <div>
              <SubTitle>{getLocale('headerLabel')}</SubTitle>
              <Title>{getLocale('headerTorTitle')}</Title>
              <Text>{getLocale('headerTorText')}</Text>
            </div>
          </div>
        </HeaderBox>
        <Box>
          <Content>
            <TorLockImage />
            <SubTitle>{getLocale('boxTorLabel')}</SubTitle>
            <Title>{getLocale('boxTorTitle')}</Title>
            <Text>{getLocale('boxTorText')}</Text>
          </Content>
          <Separator />
          <FakeButton
            href='https://support.brave.com/hc/en-us/articles/360018121491'
            target='_blank'
          >
            {getLocale('boxTorButton')}
          </FakeButton>
        </Box>
        <Box>
          <Title>{getLocale('torStatus')}</Title>
          <Text>{this.torStatus}</Text>
        </Box>
      </Grid2Columns>
    )
  }
}
