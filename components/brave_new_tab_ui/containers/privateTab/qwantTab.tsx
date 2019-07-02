/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Grid2Columns,
  Box,
  HeaderBox,
  Title,
  SubTitle,
  Text,
  TorImage,
  Separator,
  FakeButton
} from '../../components/private'
import TorContent from './torContent'
// Helpers
import { getLocale } from '../../../common/locale'

// Assets
const privateWindowImg = require('../../../img/newtab/private-window.svg')

export default class QwantTab extends React.PureComponent<{}, {}> {
  render () {
    return (
      <Grid2Columns>
        <HeaderBox>
          <div>
            <TorImage src={privateWindowImg} />
            <div>
              <SubTitle>{getLocale('headerLabel')}</SubTitle>
              <Title>{getLocale('headerTitle')}</Title>
              <Text>{getLocale('headerText')}</Text>
            </div>
          </div>
        </HeaderBox>
        <Box>
          <TorContent />
          <Separator />
          <FakeButton
            href='https://support.brave.com/hc/en-us/articles/360018121491'
            target='_blank'
          >
            {getLocale('boxTorButton')}
          </FakeButton>
        </Box>
      </Grid2Columns>
    )
  }
}
