/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components group
import ShieldsHeader from './header'
import ShieldsInterfaceControls from './interfaceControls'
import ShieldsPrivacyControls from './privacyControls'
import ShieldsFooter from './footer'

// Utils
import { ShieldsPanel } from 'brave-ui/features/shields'
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { Tab } from '../../types/state/shieldsPannelState'

interface Props {
  actions: {
    shieldsToggled: shieldActions.ShieldsToggled
    blockAdsTrackers: shieldActions.BlockAdsTrackers
    httpsEverywhereToggled: shieldActions.HttpsEverywhereToggled
    blockJavaScript: shieldActions.BlockJavaScript
    blockFingerprinting: shieldActions.BlockFingerprinting
    blockCookies: shieldActions.BlockCookies
    allowScriptOriginsOnce: shieldActions.AllowScriptOriginsOnce
    changeNoScriptSettings: shieldActions.ChangeNoScriptSettings
    changeAllNoScriptSettings: shieldActions.ChangeAllNoScriptSettings
  }
  shieldsPanelTabData: Tab
}

export default class BraveShields extends React.Component<Props, {}> {
  render () {
    const { shieldsPanelTabData, actions } = this.props
    if (!shieldsPanelTabData) {
      return null
    }

    return (
      <ShieldsPanel data-test-id='brave-shields-panel'>
        <ShieldsHeader
          tabData={shieldsPanelTabData}
          shieldsToggled={actions.shieldsToggled}
        />
        <ShieldsInterfaceControls
          url={shieldsPanelTabData.url}
          hostname={shieldsPanelTabData.hostname}
          braveShields={shieldsPanelTabData.braveShields}
          ads={shieldsPanelTabData.ads}
          adsBlocked={shieldsPanelTabData.adsBlocked}
          adsBlockedResources={shieldsPanelTabData.adsBlockedResources}
          blockAdsTrackers={actions.blockAdsTrackers}
          trackers={shieldsPanelTabData.trackers}
          trackersBlocked={shieldsPanelTabData.trackersBlocked}
          trackersBlockedResources={shieldsPanelTabData.trackersBlockedResources}
          httpsRedirected={shieldsPanelTabData.httpsRedirected}
          httpUpgradableResources={shieldsPanelTabData.httpUpgradableResources}
          httpsRedirectedResources={shieldsPanelTabData.httpsRedirectedResources}
          httpsEverywhereToggled={actions.httpsEverywhereToggled}
        />
        <ShieldsPrivacyControls
          url={shieldsPanelTabData.url}
          hostname={shieldsPanelTabData.hostname}
          origin={shieldsPanelTabData.origin}
          braveShields={shieldsPanelTabData.braveShields}
          fingerprinting={shieldsPanelTabData.fingerprinting}
          fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
          fingerprintingBlockedResources={shieldsPanelTabData.fingerprintingBlockedResources}
          blockFingerprinting={actions.blockFingerprinting}
          javascript={shieldsPanelTabData.javascript}
          javascriptBlocked={shieldsPanelTabData.javascriptBlocked}
          javascriptBlockedResources={shieldsPanelTabData.javascriptBlockedResources}
          noScriptInfo={shieldsPanelTabData.noScriptInfo}
          changeAllNoScriptSettings={actions.changeAllNoScriptSettings}
          allowScriptOriginsOnce={actions.allowScriptOriginsOnce}
          changeNoScriptSettings={actions.changeNoScriptSettings}
          blockJavaScript={actions.blockJavaScript}
          blockCookies={actions.blockCookies}
          cookies={shieldsPanelTabData.cookies}
        />
        <ShieldsFooter />
      </ShieldsPanel>
    )
  }
}
