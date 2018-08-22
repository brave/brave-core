/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

const SUPPORTED_VIDEO_EXTENSIONS = [
  'm4v',
  'mkv',
  'mov',
  'mp4',
  'ogv',
  'webm'
]

const SUPPORTED_AUDIO_EXTENSIONS = [
  'aac',
  'mp3',
  'ogg',
  'wav',
  'm4a'
]

// Given 'foo.txt', returns 'txt'
// Given eg. null, undefined, '', or 'README', returns null
const getExtension = (filename: string) => {
  if (!filename) return null
  const ix = filename.lastIndexOf('.')
  if (ix < 0) return null
  return filename.substring(ix + 1)
}

interface Props {
  torrent: TorrentObj
  ix: number
}

export default class MediaViewer extends React.PureComponent<Props, {}> {
  render () {
    const { torrent, ix } = this.props

    const file = torrent.files ? torrent.files[ix] : undefined
    const fileURL = torrent.serverURL && (torrent.serverURL + '/' + ix)

    const fileExt = file && getExtension(file.name)
    const isVideo = fileExt ? SUPPORTED_VIDEO_EXTENSIONS.includes(fileExt) : false
    const isAudio = fileExt ? SUPPORTED_AUDIO_EXTENSIONS.includes(fileExt) : false

    let content
    if (!file || !torrent.serverURL) {
      content = <div className='loading'>Loading Media</div>
    } else if (isVideo) {
      content = <video id='video' src={fileURL} autoPlay={true} controls={true} />
    } else if (isAudio) {
      content = <audio id='audio' src={fileURL} autoPlay={true} controls={true} />
    } else {
      // For security, sandbox and disallow scripts.
      // We need allow-same-origin so that the iframe can load from
      // http://localhost:...
      content = <iframe id='other' src={fileURL} sandbox='allow-same-origin' />
    }

    return (
      <div className='mediaViewer'>
        {content}
      </div>
    )
  }
}
