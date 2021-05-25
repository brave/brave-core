/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

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
  SetAdvancedViewFirstAccess,
  ShieldsReady
} from '../types/actions/shieldsPanelActions'
import { SetStoreSettingsChange } from '../types/actions/settingsActions'
import { SettingsData } from '../types/other/settingsTypes'

// Helpers
import { shieldsHasFocus } from '../helpers/shieldsUtils'

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
    setStoreSettingsChange: SetStoreSettingsChange
    shieldsReady: ShieldsReady
  }
  shieldsPanelTabData: Tab
  persistentData: PersistentData
  settingsData: SettingsData
}

interface State {
  showReadOnlyView: boolean
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showReadOnlyView: false
    }
  }

  toggleReadOnlyView = () => {
    this.setState({ showReadOnlyView: !this.state.showReadOnlyView })
  }

  toggleAdvancedView = () => {
    const { showAdvancedView } = this.props.settingsData
    shieldsAPI.setViewPreferences({ showAdvancedView: !showAdvancedView })
      .then(() => this.props.actions.setStoreSettingsChange({ showAdvancedView: !showAdvancedView }))
      .catch((err) => console.log('[Shields] Unable to toggle advanced view interface:', err))
  }

  componentDidMount () {
    this.props.actions.shieldsReady()
  }

  componentDidUpdate (prevProps: Props) {
    // If current window is not focused, close Shields immediately.
    // See https://github.com/brave/brave-browser/issues/6601.
    const { url }: Tab = this.props.shieldsPanelTabData
    if (shieldsHasFocus(url) === false) {
      window.close()
    }
  }

  render () {
    const { shieldsPanelTabData, persistentData, settingsData, actions } = this.props
    const { showReadOnlyView } = this.state
    if (!shieldsPanelTabData) {
      return null
    }
    return settingsData.showAdvancedView
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
