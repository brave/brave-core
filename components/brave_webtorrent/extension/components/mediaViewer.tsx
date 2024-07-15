// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Constants
import { TorrentObj } from '../constants/webtorrentState'
import { getFileType, isMedia } from '../utils/fileType'
import Spinner from './spinner'

const getSelectedFile = (torrent: TorrentObj, ix: number) => torrent.files?.[ix]

const Container = styled.div`
  video,
  audio,
  image-rendering,
  object,
  iframe,
  img {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    max-height: 100%;
    max-width: 100%;
    margin: auto;
  }

  object,
  iframe {
    width: 100%;
    height: 100%;
  }

  iframe {
    border: 0;
  }
`

const LoadingContainer = styled.div`
  color: var(--text3);
  margin-top: 40px;
  display: flex;
  gap: 8px;
  justify-content: center;
  align-items: center;
  width: 100%;
`

interface Props {
  torrent: TorrentObj
  ix: number
}

const playMedia = (element: HTMLMediaElement) =>
  element.play().catch((err) => console.error('Autoplay failed', err))
const setMediaElementRef = (element: HTMLMediaElement | null) => {
  if (!element) return
  if (element.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA) {
    playMedia(element)
    return
  }
  element.addEventListener('loadeddata', () => playMedia(element), {
    once: true
  })
}

export default function MediaViewer({ torrent, ix }: Props) {
  const file = getSelectedFile(torrent, ix)
  const fileType = getFileType(file)
  const fileURL = torrent.serverURL && torrent.serverURL + '/' + ix
  const loading = !file || !fileURL

  React.useEffect(() => {
    document.body.style.backgroundColor = isMedia(fileType)
      ? 'rgb(0, 0, 0)'
      : ''

    // Reset background color when unmounted.
    return () => {
      document.body.style.backgroundColor = ''
    }
  }, [fileType])

  return (
    <Container>
      {loading ? (
        <LoadingContainer>
          <Spinner />
          Loading media...
        </LoadingContainer>
      ) : (
        <>
          {fileType === 'video' && (
            <video
              id='video'
              src={fileURL}
              ref={setMediaElementRef}
              controls
            />
          )}
          {fileType === 'audio' && (
            <audio
              id='audio'
              src={fileURL}
              ref={setMediaElementRef}
              controls
            />
          )}
          {fileType === 'image' && (
            <img
              id='image'
              src={fileURL}
            />
          )}
          {fileType === 'pdf' && (
            <object
              id='object'
              type='application/pdf'
              data={fileURL}
            />
          )}
          {fileType === 'iframe' && (
            <iframe
              id='iframe'
              src={fileURL}
              sandbox='allow-same-origin'
            />
          )}
          {fileType === 'unknown' && <div>Unsupported file type</div>}
        </>
      )}
    </Container>
  )
}
