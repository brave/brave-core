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

// API
import * as shieldsAPI from '../background/api/shieldsAPI'

// Types
import { Tab, PersistentData } from '../types/state/shieldsPannelState'
import {
  ShieldsToggled,
  HideCosmeticElements,
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
    hideCosmeticElements: HideCosmeticElements
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
  settings: any
}

interface State {
  showReadOnlyView: boolean
  showAdvancedView: boolean
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showReadOnlyView: false,
      showAdvancedView: props.settings.showAdvancedView
    }
  }

  toggleReadOnlyView = () => {
    this.setState({ showReadOnlyView: !this.state.showReadOnlyView })
  }

  toggleAdvancedView = () => {
    const { showAdvancedView } = this.state
    shieldsAPI.setViewPreferences({ showAdvancedView: !showAdvancedView })
      // change local state so the component can trigger an update
      // otherwise change will be visible only after shields closes
      .then(() => this.setState({ showAdvancedView: !showAdvancedView }))
      .catch((err) => console.log('[Shields] Unable to toggle advanced view interface:', err))
  }

  render () {
    const { shieldsPanelTabData, persistentData, actions } = this.props
    const { showAdvancedView, showReadOnlyView } = this.state
    if (!shieldsPanelTabData) {
      return null
    }
    return showAdvancedView
      ? (
        <AdvancedView
          shieldsPanelTabData={shieldsPanelTabData}
          persistentData={persistentData}
          toggleAdvancedView={this.toggleAdvancedView}
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
          toggleAdvancedView={this.toggleAdvancedView}
          toggleReadOnlyView={this.toggleReadOnlyView}
        />
      )
  }
}
