/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import BraveShieldsHeader from './braveShieldsHeader'
import BraveShieldsStats from './braveShieldsStats'
import BraveShieldsControls from './braveShieldsControls'
import BraveShieldsFooter from './braveShieldsFooter'
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { Tab } from '../../types/state/shieldsPannelState'

interface BraveShieldsProps {
  actions: {
    shieldsToggled: shieldActions.ShieldsToggled
    blockAdsTrackers: shieldActions.BlockAdsTrackers
    controlsToggled: shieldActions.ControlsToggled
    httpsEverywhereToggled: shieldActions.HttpsEverywhereToggled
    javascriptToggled: shieldActions.JavascriptToggled
    blockFingerprinting: shieldActions.BlockFingerprinting
    blockCookies: shieldActions.BlockCookies
    allowScriptOriginsOnce: shieldActions.AllowScriptOriginsOnce
    changeNoScriptSettings: shieldActions.ChangeNoScriptSettings
  }
  shieldsPanelTabData: Tab
}

export default class BraveShields extends React.Component<BraveShieldsProps, {}> {
  render () {
    const { shieldsPanelTabData, actions } = this.props

    if (!shieldsPanelTabData) {
      return null
    }

    return (
      <div data-test-id='brave-shields-panel'>
        <BraveShieldsHeader
          braveShields={shieldsPanelTabData.braveShields}
          shieldsToggled={actions.shieldsToggled}
          hostname={shieldsPanelTabData.hostname}
        />
        <BraveShieldsStats
          braveShields={shieldsPanelTabData.braveShields}
          adsBlocked={shieldsPanelTabData.adsBlocked}
          trackersBlocked={shieldsPanelTabData.trackersBlocked}
          httpsRedirected={shieldsPanelTabData.httpsRedirected}
          javascriptBlocked={shieldsPanelTabData.javascriptBlocked}
          fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
        />
        <BraveShieldsControls
          braveShields={shieldsPanelTabData.braveShields}
          blockAdsTrackers={actions.blockAdsTrackers}
          ads={shieldsPanelTabData.ads}
          trackers={shieldsPanelTabData.trackers}
          httpUpgradableResources={shieldsPanelTabData.httpUpgradableResources}
          javascript={shieldsPanelTabData.javascript}
          controlsToggled={actions.controlsToggled}
          httpsEverywhereToggled={actions.httpsEverywhereToggled}
          javascriptToggled={actions.javascriptToggled}
          controlsOpen={shieldsPanelTabData.controlsOpen}
          fingerprinting={shieldsPanelTabData.fingerprinting}
          blockFingerprinting={actions.blockFingerprinting}
          cookies={shieldsPanelTabData.cookies}
          blockCookies={actions.blockCookies}
          noScriptInfo={shieldsPanelTabData.noScriptInfo}
          allowScriptOriginsOnce={actions.allowScriptOriginsOnce}
          changeNoScriptSettings={actions.changeNoScriptSettings}
        />
        <BraveShieldsFooter />
      </div>
    )
  }
}
