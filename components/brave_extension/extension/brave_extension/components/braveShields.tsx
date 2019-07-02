/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// CSS normalizer
import 'emptykit.css'

// Components group
import AdvancedView from './advancedView'

// Types
import { Tab, PersistentData } from '../types/state/shieldsPannelState'
import {
  ShieldsToggled,
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
} from '../types/actions/shieldsPanelActions'

interface Props {
  actions: {
    shieldsToggled: ShieldsToggled
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
}

export default class Shields extends React.PureComponent<Props, {}> {
  render () {
    const { shieldsPanelTabData, persistentData, actions } = this.props
    if (!shieldsPanelTabData) {
      return null
    }

    return (
      <AdvancedView
        shieldsPanelTabData={shieldsPanelTabData}
        persistentData={persistentData}
        actions={actions}
      />
    )
  }
}
