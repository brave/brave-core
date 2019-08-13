/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Assets
require('../../../fonts/muli.css')
require('../styles/styles.css')

// Components
import { StyledTorrentViewer } from '../styles/styles'
import TorrentViewerHeader from './torrentViewerHeader'
import TorrentStatus from './torrentStatus'
import TorrentFileList from './torrentFileList'
import TorrentViewerFooter from './torrentViewerFooter'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

interface Props {
  actions: any
  tabId: number
  name?: string | string[]
  torrentId: string
  errorMsg?: string
  torrent?: TorrentObj
}

export default class TorrentViewer extends React.PureComponent<Props, {}> {
  render () {
    const { actions, tabId, name, torrentId, torrent, errorMsg } = this.props

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
        <TorrentStatus torrent={torrent} errorMsg={errorMsg} />
        <TorrentFileList torrentId={torrentId} torrent={torrent} />
        <TorrentViewerFooter torrent={torrent} />
      </StyledTorrentViewer>
    )
  }
}
