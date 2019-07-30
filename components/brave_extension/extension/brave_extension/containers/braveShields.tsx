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
    shieldsReady: ShieldsReady
  }
  shieldsPanelTabData: Tab
  persistentData: PersistentData
  settings: any
}

interface State {
  url: string
}

export default class Shields extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      url: ''
    }
  }

  componentDidMount () {
    this.getActiveTabUrl()
  }

  get pageHasDownloadableVideo () {
    return this.state.url.startsWith('https://www.youtube.com/watch')
  }

  getActiveTabUrl = () => {
    chrome.tabs.query({ active: true, currentWindow: true }, tabs => {
      const activeTab = tabs[0]
      if (activeTab.url) {
        this.setState({ url: activeTab.url })
      }
    })
  }

  onClickDownloadVideo = (url: string) => {
    chrome.bravePlaylists.requestDownload(url)
  }

  render () {
    const { url } = this.state

    return this.pageHasDownloadableVideo
      ? (
        <div>
          <h1>This page has a video you can download</h1>
          <button onClick={this.onClickDownloadVideo.bind(this, url)}>Click here to download</button>
        </div>
      ) : (
        <h1>Nothing to see here. Go to a YT video to see the magic</h1>
      )
  }
}
