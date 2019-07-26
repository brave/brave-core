/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import TorrentViewerHeader from '../../../brave_webtorrent/extension/components/torrentViewerHeader'
import { TestThemeProvider } from 'brave-ui/theme'

describe('torrentViewerHeader component', () => {
  const startTorrentMock = jest.fn()
  const stopDownloadMock = jest.fn()
  const tabId = 123
  const torrentId = 'id'

  describe('torrentViewerHeader dumb component', () => {
    it('renders the component with start download button if no torrent object', () => {
      const wrapper = shallow(
        <TestThemeProvider>
          <TorrentViewerHeader
            tabId={tabId}
            torrentId={torrentId}
            onStartTorrent={startTorrentMock}
            onStopDownload={stopDownloadMock}
          />
        </TestThemeProvider>
      )
      expect(wrapper.html()).toEqual(expect.stringContaining('Start Torrent'))
    })

    it('renders the component with stop download button if torrent object is passed', () => {
      const wrapper = shallow(
        <TestThemeProvider>
          <TorrentViewerHeader
            tabId={tabId}
            torrentId={torrentId}
            torrent={torrentObj}
            onStartTorrent={startTorrentMock}
            onStopDownload={stopDownloadMock}
          />
        </TestThemeProvider>
      )
      expect(wrapper.html()).toEqual(expect.stringContaining('Stop Torrent'))
    })

    it('renders the component with torrent title', () => {
      const wrapper = shallow(
        <TestThemeProvider>
          <TorrentViewerHeader
            tabId={tabId}
            torrentId={torrentId}
            torrent={torrentObj}
            name={torrentObj.name}
            onStartTorrent={startTorrentMock}
            onStopDownload={stopDownloadMock}
          />
        </TestThemeProvider>
      )
      expect(wrapper.html()).toEqual(expect.stringContaining(torrentObj.name))
    })
  })
})
