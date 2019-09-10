/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Types
import { Tab } from '../../types/state/shieldsPannelState'

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
} from 'brave-ui/features/shields'

// Group components
import InterfaceControls from './interfaceControls'
import PrivacyControls from './privacyControls'

// Helpers
import { getLocale } from '../../background/api/localeAPI'
import { getFavicon } from '../../helpers/shieldsUtils'

interface Props {
  shieldsPanelTabData: Tab
  toggleReadOnlyView: (event?: React.MouseEvent) => void
}

export default class ShieldsReadOnlyView extends React.PureComponent<Props, {}> {
  get favicon (): string {
    const { url } = this.props.shieldsPanelTabData
    return getFavicon(url)
  }
  render () {
    const { shieldsPanelTabData, toggleReadOnlyView } = this.props
    return (
      <ShieldsPanel style={{ width: '370px' }}>
        <BlockedListHeader>
          <Favicon src={this.favicon} />
          <SiteInfoText>{shieldsPanelTabData.hostname}</SiteInfoText>
        </BlockedListHeader>
        <StaticHeader>
          <BlockedInfoRowText>{getLocale('shieldsExplanation')}</BlockedInfoRowText>
        </StaticHeader>
        <StaticResourcesControls>
          <StaticResourcesContainer>
          <InterfaceControls
            // Ads/Trackers
            ads={shieldsPanelTabData.ads}
            adsBlocked={shieldsPanelTabData.adsBlocked}
            adsBlockedResources={shieldsPanelTabData.adsBlockedResources}
            trackers={shieldsPanelTabData.trackers}
            trackersBlocked={shieldsPanelTabData.trackersBlocked}
            trackersBlockedResources={shieldsPanelTabData.trackersBlockedResources}
            // HTTPS Upgrades
            httpsRedirected={shieldsPanelTabData.httpsRedirected}
            httpUpgradableResources={shieldsPanelTabData.httpUpgradableResources}
            httpsRedirectedResources={shieldsPanelTabData.httpsRedirectedResources}
          />
          <PrivacyControls
            // JavaScript
            javascript={shieldsPanelTabData.javascript}
            javascriptBlocked={shieldsPanelTabData.javascriptBlocked}
            noScriptInfo={shieldsPanelTabData.noScriptInfo}
            // Fingerprinting
            fingerprinting={shieldsPanelTabData.fingerprinting}
            fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
            fingerprintingBlockedResources={shieldsPanelTabData.fingerprintingBlockedResources}
          />
          </StaticResourcesContainer>
        </StaticResourcesControls>
        <BlockedListFooter>
          <ShieldsButton level='primary' type='accent' onClick={toggleReadOnlyView} text={getLocale('goBack')} />
        </BlockedListFooter>
      </ShieldsPanel>
    )
  }
}
