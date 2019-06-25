/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  ShieldsPanel,
  BlockedListHeader,
  Favicon,
  SiteInfoText,
  StaticHeader,
  StaticResourcesControls,
  StaticResourcesContainer,
  BlockedInfoRowText,
  BlockedListFooter,
  ShieldsButton
} from '../../../../../src/features/shields'

// Group components
import InterfaceControls from './interfaceControls'
import PrivacyControls from './privacyControls'

// Helpers
import { getLocale } from '../../fakeLocale'

interface Props {
  hostname: string
  favicon: string
  onClose: (event?: any) => void
}

export default class ShieldsReadOnlyView extends React.PureComponent<Props, {}> {
  render () {
    const { favicon, hostname, onClose } = this.props
    return (
      <ShieldsPanel style={{ width: '370px' }}>
        <BlockedListHeader>
          <Favicon src={favicon} />
          <SiteInfoText>{hostname}</SiteInfoText>
        </BlockedListHeader>
        <StaticHeader>
          <BlockedInfoRowText>{getLocale('shieldsExplanation')}</BlockedInfoRowText>
        </StaticHeader>
        <StaticResourcesControls>
          <StaticResourcesContainer>
            <InterfaceControls />
            <PrivacyControls />
          </StaticResourcesContainer>
        </StaticResourcesControls>
        <BlockedListFooter>
          <ShieldsButton level='primary' type='accent' onClick={onClose} text={getLocale('goBack')} />
        </BlockedListFooter>
      </ShieldsPanel>
    )
  }
}
