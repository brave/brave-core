// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Assets

// Components
import TorrentFileList from './torrentFileList'
import TorrentStatus from './torrentStatus'
import TorrentViewerFooter from './torrentViewerFooter'
import TorrentViewerHeader from './torrentViewerHeader'

// Constants
import styled from 'styled-components'
import { TorrentObj, TorrentState } from '../constants/webtorrentState'

interface Props {
  actions: any
  name?: string | string[]
  torrent?: TorrentObj
  torrentState: TorrentState
}

const StyledTorrentViewer = styled.div`
  display: flex;
  flex-direction: column;
  gap: 40px;

  min-height: 100%;
  padding: 50px;
  font-family: Muli, sans-serif;

  max-width: 1280px;
  margin: 0 auto;

  color: var(--text1);

  button > div {
    font-weight: 600;
  }
`

export default function TorrentViewer({
  actions,
  name,
  torrent,
  torrentState
}: Props) {
  const { torrentId, tabId, errorMsg, infoHash } = torrentState
  return (
    <StyledTorrentViewer>
      <TorrentViewerHeader
        name={name}
        torrent={torrent}
        torrentId={torrentId}
        tabId={tabId}
        onStartTorrent={actions.startTorrent}
        onStopDownload={actions.stopDownload}
      />
      <TorrentStatus
        torrent={torrent}
        errorMsg={errorMsg}
      />
      <TorrentFileList
        torrentId={torrentId}
        torrent={torrent}
        onSaveAllFiles={() => actions.saveAllFiles(infoHash)}
      />
      <TorrentViewerFooter torrent={torrent} />
    </StyledTorrentViewer>
  )
}
