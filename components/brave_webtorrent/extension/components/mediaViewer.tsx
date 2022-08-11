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

type FileType = 'video' | 'audio' | 'image' | 'pdf' | 'iframe' | 'unknown'
const mediaTypes: FileType[] = ['video', 'audio', 'image']

// Given 'foo.txt', returns 'txt'
// undefined or README return ''
const getExtension = (filename?: string) => {
  if (!filename) return ''
  const ix = filename.lastIndexOf('.')
  if (ix < 0) return ''
  return filename.substring(ix + 1)
}

const getSelectedFile = (torrent: TorrentObj, ix: number) => torrent.files?.[ix]

const getFileType = (file?: File): FileType => {
  const extension = getExtension(file?.name)
  if (SUPPORTED_VIDEO_EXTENSIONS.includes(extension)) return 'video'
  if (SUPPORTED_AUDIO_EXTENSIONS.includes(extension)) return 'audio'
  if (SUPPORTED_IMAGE_EXTENSIONS.includes(extension)) return 'image'
  if (SUPPORTED_PDF_EXTENSIONS.includes(extension)) return 'pdf'
  if (SUPPORTED_IFRAME_EXTENSIONS.includes(extension)) return 'iframe'
  return 'unknown'
}

interface Props {
  torrent: TorrentObj
  ix: number
}

const playMedia = (element: HTMLMediaElement) => element.play().catch(err => console.error('Autoplay failed', err))
const setMediaElementRef = (element: HTMLMediaElement | null) => {
  if (!element) return
  if (element.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA) {
    playMedia(element)
    return
  }
  element.addEventListener('loadeddata', () => playMedia(element), { once: true })
}

export default function MediaViewer ({ torrent, ix }: Props) {
  const file = getSelectedFile(torrent, ix)
  const fileType = getFileType(file)
  const fileURL = torrent.serverURL && torrent.serverURL + '/' + ix
  const loading = !file || !fileURL

  React.useEffect(() => {
    const isMedia = mediaTypes.includes(fileType)
    document.body.style.backgroundColor = isMedia ? 'rgb(0, 0, 0)' : ''

    // Reset background color when unmounted.
    return () => {
      document.body.style.backgroundColor = ''
    }
  }, [file, fileType])

  return <div className='mediaViewer'>
    {loading
      ? <div>Loading media...</div>
      : <>
        {fileType === 'video' && <video id='video' src={fileURL} ref={setMediaElementRef} controls />}
        {fileType === 'audio' && <audio id='audio' src={fileURL} ref={setMediaElementRef} controls />}
        {fileType === 'image' && <img id='image' src={fileURL} />}
        {fileType === 'pdf' && <object id='object' type='application/pdf' data={fileURL}/>}
        {fileType === 'iframe' && <iframe id='iframe' src={fileURL} sandbox='allow-same-origin' />}
        {fileType === 'unknown' && <div>Unsupported file type</div>}
      </>}
  </div>
}
