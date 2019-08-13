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
      expect(wrapper.find(StyledTorrentViewer)).toHaveLength(1)
    })
  })
})
