/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import TorrentViewer from '../../../brave_webtorrent/extension/components/torrentViewer'
import TorrentFileList from '../../../brave_webtorrent/extension/components/torrentFileList'
import TorrentStatus from '../../../brave_webtorrent/extension/components/torrentStatus'
import TorrentViewerFooter from '../../../brave_webtorrent/extension/components/torrentViewerFooter'
import TorrentViewerHeader from '../../../brave_webtorrent/extension/components/torrentViewerHeader'

describe('torrentViewer component', () => {
  describe('torrentViewer dumb component', () => {
    it('renders the component', () => {
      const torrentState: TorrentState = {
        tabId: 1,
        torrentId: 'id'
      }
      const name = 'name'
      const wrapper = shallow(
        <TorrentViewer
          actions={{}}
          name={name}
          torrent={torrentObj}
          torrentState={torrentState}
        />
      )
      expect(wrapper.find(TorrentViewerHeader)).toHaveLength(1)
      expect(wrapper.find(TorrentStatus)).toHaveLength(1)
      expect(wrapper.find(TorrentFileList)).toHaveLength(1)
      expect(wrapper.find(TorrentViewerFooter)).toHaveLength(1)
    })
  })
})
