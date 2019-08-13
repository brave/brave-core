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
import { TorrentObj, TorrentState } from '../constants/webtorrentState'

interface Props {
  actions: any
  name?: string | string[]
  torrent?: TorrentObj
  torrentState: TorrentState
}

export default class TorrentViewer extends React.PureComponent<Props, {}> {
  render () {
    const { actions, name, torrent, torrentState } = this.props
    const { torrentId, tabId, errorMsg, infoHash } = torrentState

    const onSaveAllFiles = () => actions.saveAllFiles(infoHash)

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
        <TorrentFileList
          torrentId={torrentId}
          torrent={torrent}
          onSaveAllFiles={onSaveAllFiles}
        />
        <TorrentViewerFooter torrent={torrent} />
      </StyledTorrentViewer>
    )
  }
}
