/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ShieldsPanel } from '../../components'

// Components group
import Header from './header'
import InterfaceControls from './interfaceControls'
import PrivacyControls from './privacyControls'
import Footer from './footer'
import WebCompatWarning from './overlays/webCompatWarningOverlay'

// Types
import { Tab, PersistentData } from '../../types/state/shieldsPannelState'
import { isShieldsEnabled, getFavicon } from '../../helpers/shieldsUtils'
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
}

interface State {
  isBlockedListOpen: boolean
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { isBlockedListOpen: false }
  }

  get favicon (): string {
    const { url } = this.props.shieldsPanelTabData
    return getFavicon(url)
  }

  get isShieldsEnabled (): boolean {
    const { braveShields } = this.props.shieldsPanelTabData
    return isShieldsEnabled(braveShields)
  }

  setBlockedListOpen = () => {
    this.setState({ isBlockedListOpen: !this.state.isBlockedListOpen })
  }

  render () {
    const { shieldsPanelTabData, persistentData, toggleAdvancedView, actions } = this.props
    const { isBlockedListOpen } = this.state

    if (!shieldsPanelTabData) {
      return null
    }

    return (
      <ShieldsPanel data-test-id='brave-shields-panel' style={{ width: '370px' }}>
        {
          persistentData.isFirstAccess
            && <WebCompatWarning setAdvancedViewFirstAccess={actions.setAdvancedViewFirstAccess} />
        }
        <Header
          enabled={this.isShieldsEnabled}
          favicon={this.favicon}
          hostname={shieldsPanelTabData.hostname}
          isBlockedListOpen={isBlockedListOpen}
          adsBlocked={shieldsPanelTabData.adsBlocked}
          trackersBlocked={shieldsPanelTabData.trackersBlocked}
          httpsUpgrades={shieldsPanelTabData.httpsRedirected}
          scriptsBlocked={shieldsPanelTabData.javascriptBlocked}
          fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
          shieldsToggled={actions.shieldsToggled}
          reportBrokenSite={actions.reportBrokenSite}
        />
        {
          this.isShieldsEnabled && (
            <>
              <InterfaceControls
                // Global props
                isBlockedListOpen={isBlockedListOpen}
                setBlockedListOpen={this.setBlockedListOpen}
                hostname={shieldsPanelTabData.hostname}
                favicon={this.favicon}
                // Ads/Trackers
                ads={shieldsPanelTabData.ads}
                adsBlocked={shieldsPanelTabData.adsBlocked}
                adsBlockedResources={shieldsPanelTabData.adsBlockedResources}
                trackers={shieldsPanelTabData.trackers}
                trackersBlocked={shieldsPanelTabData.trackersBlocked}
                trackersBlockedResources={shieldsPanelTabData.trackersBlockedResources}
                firstPartyCosmeticFiltering={shieldsPanelTabData.firstPartyCosmeticFiltering}
                blockAdsTrackers={actions.blockAdsTrackers}
                // HTTPS Upgrades
                httpsRedirected={shieldsPanelTabData.httpsRedirected}
                httpUpgradableResources={shieldsPanelTabData.httpUpgradableResources}
                httpsRedirectedResources={shieldsPanelTabData.httpsRedirectedResources}
                httpsEverywhereToggled={actions.httpsEverywhereToggled}
              />
              <PrivacyControls
                // Global props
                isBlockedListOpen={isBlockedListOpen}
                setBlockedListOpen={this.setBlockedListOpen}
                hostname={shieldsPanelTabData.hostname}
                favicon={this.favicon}
                // JavaScript
                javascript={shieldsPanelTabData.javascript}
                javascriptBlocked={shieldsPanelTabData.javascriptBlocked}
                noScriptInfo={shieldsPanelTabData.noScriptInfo}
                blockJavaScript={actions.blockJavaScript}
                allowScriptOriginsOnce={actions.allowScriptOriginsOnce}
                setScriptBlockedCurrentState={actions.setScriptBlockedCurrentState}
                setGroupedScriptsBlockedCurrentState={actions.setGroupedScriptsBlockedCurrentState}
                setAllScriptsBlockedCurrentState={actions.setAllScriptsBlockedCurrentState}
                setFinalScriptsBlockedState={actions.setFinalScriptsBlockedState}
                // Cookies
                blockCookies={actions.blockCookies}
                cookies={shieldsPanelTabData.cookies}
                // Fingerprinting
                fingerprinting={shieldsPanelTabData.fingerprinting}
                fingerprintingBlocked={shieldsPanelTabData.fingerprintingBlocked}
                fingerprintingBlockedResources={shieldsPanelTabData.fingerprintingBlockedResources}
                blockFingerprinting={actions.blockFingerprinting}
              />
            </>
          )
        }
        <Footer
          enabled={this.isShieldsEnabled}
          isBlockedListOpen={isBlockedListOpen}
          toggleAdvancedView={toggleAdvancedView}
        />
      </ShieldsPanel>
    )
  }
}
