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

import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'

interface State {
  url: string
  playlists: any
}

export default class Shields extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      url: '',
      playlists: []
    }
    // fetch playlists right away
    this.getPlaylist()
  }

  getPlaylist = () => {
    chrome.bravePlaylists.getAllPlaylists(playlists => {
      this.setState({ playlists })
    })
  }

  componentDidMount () {
    this.getActiveTabUrl()
    chrome.bravePlaylists.onPlaylistsChanged.addListener((changeType, id) => {
      this.getPlaylist()
    })
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

  componentDidUpdate (prevProps: any, prevState: any) {
    if (JSON.stringify(prevState.playlists) !== JSON.stringify(this.state.playlists)) {
      this.getPlaylist()
    }
  }

  getPlaylistHeader = (): Cell[] => {
    return [
      { content: 'NAME' }
    ]
  }

  get lazyButtonStyle () {
    const lazyButtonStyle: any = {
      alignItems: 'center',
      WebkitAppearance: 'none',
      width: '50px',
      height: '50px',
      display: 'flex',
      borderRadius: '4px'
    }
    return lazyButtonStyle
  }

  getPlaylistRows = (playlist?: any): Row[] | undefined => {
    if (playlist == null) {
      return
    }

    return playlist.map((item: any, index: any): any => {
      // console.log('video file', item.videoMediaFilePath)
      // console.log('audio file', item.audioMediaFilePath)
      const cell: Row = {
        content: [
          { content: (
            <div>
              {
                item.audioMediaFilePath || item.videoMediaFilePath
                  ? (
                    <>
                      <h3>{item.playlistName}</h3>
                      <video
                        controls={true}
                        width={370}
                        poster={'file://' + item.thumbnailUrl}
                      >
                        <source src={'file://' + item.videoMediaFilePath} type='video/mp4' />
                      </video>
                      <video controls={true} style={{ display: 'none' }}><source src={'file://' + item.audioMediaFilePath} type='audio/x-m4a' /></video>
                      <hr />
                    </>
                  )
                  : <h2>The video <span style={{ color: 'red' }}>{item.playlistName}</span> is being downloaded. It will show here once available</h2>
              }
            </div>
          ) }
        ]
      }
      return cell
    })
  }

  onClickRemoveALlVideos = () => {
    console.log('nothing')
  }

  onClickRemoveVideo = () => {
    console.log('nothing')
  }

  onClickPlayVideo = () => {
    console.log('nothing')
  }

  render () {
    const { url, playlists } = this.state

    return this.pageHasDownloadableVideo
      ? (
        <div>
          <h1>This page has a video you can download</h1>
          <button
            style={Object.assign(
              {},
              this.lazyButtonStyle,
              { width: 'fit-content', fontSize: '16px' }
            )}
            onClick={this.onClickDownloadVideo.bind(this, url)}
          >
            Click here to download
          </button>
          <hr />
          <div id='adblockPage'>
            <div style={{ minHeight: '600px', width: '100%' }}>
            <h1>Your playlist</h1>
              <Table header={this.getPlaylistHeader()} rows={this.getPlaylistRows(playlists)}>
                YOUR PLAYLIST IS NOW EMPTY
              </Table>
            </div>
          </div>
        </div>
      ) : (
        <h1>Nothing to see here. Go to a YT video to see the magic</h1>
      )
  }
}
