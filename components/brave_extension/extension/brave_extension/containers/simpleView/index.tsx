/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ShieldsPanel } from '../../components'

// Components group
import Header from './header'
import Footer from './footer'

import {
  ShieldsToggled,
  ReportBrokenSite,
  BlockAdsTrackers,
  HttpsEverywhereToggled,
  BlockJavaScript,
  BlockFingerprinting,
  BlockCookies,
  AllowScriptOriginsOnce,
  SetScriptBlockedCurrentState,
  SetGroupedScriptsBlockedCurrentState,
  SetAllScriptsBlockedCurrentState,
  SetFinalScriptsBlockedState,
  SetAdvancedViewFirstAccess
} from '../../types/actions/shieldsPanelActions'

// Helpers
import { Tab, PersistentData } from '../../types/state/shieldsPanelState'
import { getFavicon, isShieldsEnabled } from '../../helpers/shieldsUtils'

interface Props {
  actions: {
    shieldsToggled: ShieldsToggled
    reportBrokenSite: ReportBrokenSite
    blockAdsTrackers: BlockAdsTrackers
    httpsEverywhereToggled: HttpsEverywhereToggled
    blockJavaScript: BlockJavaScript
    blockFingerprinting: BlockFingerprinting
    blockCookies: BlockCookies
    allowScriptOriginsOnce: AllowScriptOriginsOnce
    setScriptBlockedCurrentState: SetScriptBlockedCurrentState
    setGroupedScriptsBlockedCurrentState: SetGroupedScriptsBlockedCurrentState
    setAllScriptsBlockedCurrentState: SetAllScriptsBlockedCurrentState
    setFinalScriptsBlockedState: SetFinalScriptsBlockedState
    setAdvancedViewFirstAccess: SetAdvancedViewFirstAccess
  }
  shieldsPanelTabData: Tab
  persistentData: PersistentData
  toggleAdvancedView: () => void
  toggleReadOnlyView: () => void
}

export default class ShieldsSimpleView extends React.PureComponent<Props, {}> {
  get favicon (): string {
    const { url } = this.props.shieldsPanelTabData
    return getFavicon(url)
  }

  get isShieldsEnabled (): boolean {
    const { braveShields } = this.props.shieldsPanelTabData
    return isShieldsEnabled(braveShields)
  }

  render () {
    const { shieldsPanelTabData, actions, toggleAdvancedView, toggleReadOnlyView } = this.props
    return (
      <ShieldsPanel style={{ width: '370px' }}>
        <Header
          enabled={this.isShieldsEnabled}
          favicon={this.favicon}
          hostname={shieldsPanelTabData.hostname}
          adsBlocked={shieldsPanelTabData.adsBlocked}
          trackersBlocked={shieldsPanelTabData.trackersBlocked}
          httpsUpgrades={shieldsPanelTabData.httpsRedirected}
          scriptsBlocked={shieldsPanelTabData.javascriptBlocked}
          fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
          shieldsToggled={actions.shieldsToggled}
          toggleReadOnlyView={toggleReadOnlyView}
          reportBrokenSite={actions.reportBrokenSite}
        />
        <Footer
          enabled={this.isShieldsEnabled}
          toggleAdvancedView={toggleAdvancedView}
        />
      </ShieldsPanel>
    )
  }
}
