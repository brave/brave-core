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

import { Cell } from 'brave-ui/components/dataTables/table'

const VideoFile = require('../video_file.mp4')
const AudioFile = require('../audio_file.m4a')

interface State {
  url: string
  playlists: any
}

export default class Shields extends React.PureComponent<{}, State> {
  videoRef: React.RefObject<any>
  audioRef: React.RefObject<any>
  constructor (props: {}) {
    super(props)
    this.state = {
      url: '',
      playlists: []
    }
    // fetch playlists right away
    this.getPlaylist()
    // create refs for audio/video
    // TODO: cezaraugusto this won't work with multiple videos
    // good way to solve this is to create a separate component
    // that we can then map and so encapsulate state for each of them
    this.videoRef = React.createRef()
    this.audioRef = React.createRef()
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


  onPlayVideo = () => {
    // play event is called whenever user change the video timeline
    // ensure we're in sync with audio by adopting the same current time from video
    this.audioRef.current.currentTime = this.videoRef.current.currentTime
    // the play event will take care of playing the video,
    // we just need to call the audio to play along
    this.audioRef.current.play()
  }

  onPauseVideo = () => {
    // video is paused. pause the audio too so they remain in sync
    this.audioRef.current.pause()
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
    const { url } = this.state

    return (
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
        <div style={{ minHeight: '600px', width: '100%' }}>
          <h1>Your playlist</h1>
          <video
            ref={this.videoRef}
            controls={true}
            width={370}
            poster={''}
            onPlay={this.onPlayVideo}
            onPause={this.onPauseVideo}
          >
            <source src={VideoFile} type='video/mp4' />
          </video>
          <video ref={this.audioRef} style={{ display: 'none' }}>
            <source src={AudioFile} type='audio/m4a' />
          </video>
        </div>
      </div>
    )
  }
}
