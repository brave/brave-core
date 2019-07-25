/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import { TorrentObj, File } from '../constants/webtorrentState'

const SUPPORTED_VIDEO_EXTENSIONS = ['m4v', 'mkv', 'mov', 'mp4', 'ogv', 'webm']

const SUPPORTED_AUDIO_EXTENSIONS = ['aac', 'mp3', 'ogg', 'wav', 'm4a']

// Given 'foo.txt', returns 'txt'
// Given eg. null, undefined, '', or 'README', returns null
const getExtension = (filename: string) => {
  if (!filename) return null
  const ix = filename.lastIndexOf('.')
  if (ix < 0) return null
  return filename.substring(ix + 1)
}

const getSelectedFile = (torrent: TorrentObj, ix: number) => {
  return torrent.files
    ? torrent.files[ix]
    : null
}

const fileIsVideo = (file: File | null) => {
  if (!file) return false
  const fileExt = getExtension(file.name)
  if (!fileExt) return false
  return SUPPORTED_VIDEO_EXTENSIONS.includes(fileExt)
}

const fileIsAudio = (file: File | null) => {
  if (!file) return false
  const fileExt = getExtension(file.name)
  if (!fileExt) return false
  return SUPPORTED_AUDIO_EXTENSIONS.includes(fileExt)
}

interface Props {
  torrent: TorrentObj
  ix: number
}

export default class MediaViewer extends React.PureComponent<Props, {}> {
  ref = (elem: HTMLMediaElement | null) => {
    if (!elem) return
    elem.play().catch(err => console.error('Autoplay failed', err))
  }

  componentWillReceiveProps () {
    // Set page background to black when video or audio is being viewed
    const { torrent, ix } = this.props
    const file = getSelectedFile(torrent, ix)
    const isMedia = fileIsAudio(file) || fileIsVideo(file)
    if (isMedia) {
      document.body.style.backgroundColor = 'rgb(0, 0, 0)'
    } else {
      document.body.style.backgroundColor = null
    }
  }

  componentWillUnmount () {
    document.body.style.backgroundColor = null
  }

  render () {
    const { torrent, ix } = this.props

    const file = getSelectedFile(torrent, ix)
    const isAudio = fileIsAudio(file)
    const isVideo = fileIsVideo(file)
    const fileURL = torrent.serverURL && torrent.serverURL + '/' + ix

    let content
    if (!file || !fileURL) {
      content = <div className='loading'>Loading Media</div>
    } else if (isVideo) {
      content = (
        <video id='video' src={fileURL} ref={this.ref} controls={true} />
      )
    } else if (isAudio) {
      content = (
        <audio id='audio' src={fileURL} ref={this.ref} controls={true} />
      )
    } else {
      // For security, sandbox and disallow scripts.
      // We need allow-same-origin so that the iframe can load from
      // http://localhost:...
      content = <iframe id='other' src={fileURL} sandbox='allow-same-origin' />
    }

    return <div className='mediaViewer'>{content}</div>
  }
}
