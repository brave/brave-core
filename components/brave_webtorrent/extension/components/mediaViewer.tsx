/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import { TorrentObj, File } from '../constants/webtorrentState'

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
  'flac',
  'm4a',
  'm4b',
  'm4p',
  'mp3',
  'oga',
  'ogg',
  'wav'
]

const SUPPORTED_IMAGE_EXTENSIONS = [
  'bmp',
  'gif',
  'jpeg',
  'jpg',
  'png',
  'svg'
]

const SUPPORTED_PDF_EXTENSIONS = [
  'pdf'
]

const SUPPORTED_IFRAME_EXTENSIONS = [
  'css',
  'html',
  'js',
  'md',
  'txt'
]

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

const fileIsType = (file: File | null, type: String) => {
  const supportedExtensions = type === 'video'
    ? SUPPORTED_VIDEO_EXTENSIONS
    : type === 'audio'
    ? SUPPORTED_AUDIO_EXTENSIONS
    : type === 'image'
    ? SUPPORTED_IMAGE_EXTENSIONS
    : type === 'pdf'
    ? SUPPORTED_PDF_EXTENSIONS
    : SUPPORTED_IFRAME_EXTENSIONS

  if (!file) return false
  const fileExt = getExtension(file.name)
  if (!fileExt) return false
  return supportedExtensions.includes(fileExt)
}

interface Props {
  torrent: TorrentObj
  ix: number
}

export default class MediaViewer extends React.PureComponent<Props, {}> {
  ref = (elem: HTMLMediaElement | null) => {
    if (!elem) return
    if (elem.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA) {
      this.play(elem)
    } else {
      elem.addEventListener('loadeddata', () => this.play(elem), { once: true })
    }
  }

  play (elem: HTMLMediaElement) {
    elem.play().catch(err => console.error('Autoplay failed', err))
  }

  componentWillReceiveProps () {
    // Set page background to black when video or audio is being viewed
    const { torrent, ix } = this.props
    const file = getSelectedFile(torrent, ix)
    const isMedia = fileIsType(file, 'video') || fileIsType(file, 'audio') ||
      fileIsType(file, 'image')
    if (isMedia) {
      document.body.style.backgroundColor = 'rgb(0, 0, 0)'
    } else {
      document.body.style.backgroundColor = ''
    }
  }

  componentWillUnmount () {
    document.body.style.backgroundColor = ''
  }

  render () {
    const { torrent, ix } = this.props

    const file = getSelectedFile(torrent, ix)
    const fileURL = torrent.serverURL && torrent.serverURL + '/' + ix

    let content
    if (!file || !fileURL) {
      content = <div className='loading'>Loading Media</div>
    } else if (fileIsType(file, 'video')) {
      content = (
        <video id='video' src={fileURL} ref={this.ref} controls={true} />
      )
    } else if (fileIsType(file, 'audio')) {
      content = (
        <audio id='audio' src={fileURL} ref={this.ref} controls={true} />
      )
    } else if (fileIsType(file, 'image')) {
      content = (
        <img id='image' src={fileURL} />
       )
    } else if (fileIsType(file, 'pdf')) {
      content = (
        <object id='object' type='application/pdf' data={fileURL} />
      )
    } else if (fileIsType(file, 'iframe')) {
      // For security, sandbox and disallow scripts.
      // We need allow-same-origin so that the iframe can load from
      // http://127.0.0.1:port...
      content = (
        <iframe id='iframe' src={fileURL} sandbox='allow-same-origin' />
      )
    } else {
      content = (
        <div>Unsupported file type</div>
      )
    }

    return <div className='mediaViewer'>{content}</div>
  }
}
