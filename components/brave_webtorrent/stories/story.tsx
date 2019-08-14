/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

// Components
import TorrentViewer from '../extension/components/torrentViewer'
import { TorrentState } from '../extension/constants/webtorrentState'
// Storybook
const fullPageStoryStyles: object = {
  width: '-webkit-fill-available',
  height: '-webkit-fill-available'
}

// Storybook helpers
const FullPageStory = (storyFn: any) => (
  <div style={fullPageStoryStyles}>{storyFn()}</div>
)

function consoleLog () {
  console.log(`Clicked something`)
}

const sampleTorrent = {
  name: 'Sample Torrent',
  files: [
    {
      name: 'file1.jpg',
      length: 400
    },
    {
      name: 'file2.jpg',
      length: 500
    }
  ],
  serverURL: 'brave://myfile',
  timeRemaining: 42,
  downloaded: 2000,
  uploaded: 8000,
  downloadSpeed: 200,
  uploadSpeed: 200,
  progress: 0,
  ratio: 0,
  numPeers: 4000,
  length: 4500,
  tabClients: new Set([1, 2, 4, 5])
}

const torrentState: TorrentState = {
  tabId: 1,
  torrentId: 'id'
}

// Storybook UI
storiesOf('WebTorrent', module)
  .addDecorator(FullPageStory)
  .add('Page', () => {
    return (
      <TorrentViewer
        actions={consoleLog}
        name={'Fake Torrent with really long title'}
        torrentState={torrentState}
      />
    )
  })
  .add('Page with torrent', () => {
    return (
      <TorrentViewer
        actions={consoleLog}
        torrent={sampleTorrent}
        name={'Sample Torrent Name'}
        torrentState={torrentState}
      />
    )
  })
