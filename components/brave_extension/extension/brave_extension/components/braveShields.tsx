/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// CSS normalizer
import 'emptykit.css'

// Components group
import AdvancedView from './advancedView'
import SimpleView from './simpleView'
import ReadOnlyView from './readOnlyView'

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
  SetAdvancedViewFirstAccess,
  ToggleAdvancedView
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
    toggleAdvancedView: ToggleAdvancedView
  }
  shieldsPanelTabData: Tab
  persistentData: PersistentData
}

interface State {
  showReadOnlyView: boolean
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { showReadOnlyView: false }
  }

  toggleReadOnlyView = () => {
    this.setState({ showReadOnlyView: !this.state.showReadOnlyView })
  }

  render () {
    const { shieldsPanelTabData, persistentData, actions } = this.props
    const { showReadOnlyView } = this.state
    if (!shieldsPanelTabData) {
      return null
    }
    return persistentData.advancedView
      ? (
        <AdvancedView
          shieldsPanelTabData={shieldsPanelTabData}
          persistentData={persistentData}
          actions={actions}
        />
      ) : showReadOnlyView
      ? (
        <ReadOnlyView
          shieldsPanelTabData={shieldsPanelTabData}
          toggleReadOnlyView={this.toggleReadOnlyView}
        />
      ) : (
        <SimpleView
          shieldsPanelTabData={shieldsPanelTabData}
          persistentData={persistentData}
          actions={actions}
          toggleReadOnlyView={this.toggleReadOnlyView}
        />
      )
  }
}
