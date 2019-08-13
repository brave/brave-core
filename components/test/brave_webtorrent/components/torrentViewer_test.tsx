/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import TorrentViewer from '../../../brave_webtorrent/extension/components/torrentViewer'
import { StyledTorrentViewer } from '../../../brave_webtorrent/extension/styles/styles'

describe('torrentViewer component', () => {
  describe('torrentViewer dumb component', () => {
    it('renders the component', () => {
      const tabId = 1
      const torrentId = 'id'
      const wrapper = shallow(
        <TorrentViewer
          actions={{}}
          tabId={tabId}
          torrentId={torrentId}
          torrent={torrentObj}
        />
      )
      expect(wrapper.find(StyledTorrentViewer)).toHaveLength(1)
    })
  })
})
