/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import TorrentFileList from '../../../brave_webtorrent/extension/components/torrentFileList'

const torrentId = 'id'
describe('torrentFileList component', () => {
  describe('torrentFileList dumb component', () => {
    it('renders the start torrent message if no torrent', () => {
      const wrapper = shallow(
        <TorrentFileList
          torrentId={torrentId}
        />
      )
      expect(wrapper.html()).toEqual(expect.stringContaining('Start Torrent'))
    })

    it('renders the start torrent message if no files', () => {
      const wrapper = shallow(
        <TorrentFileList
          torrentId={torrentId}
          torrent={torrentObj}
        />
      )
      expect(wrapper.html()).toEqual(expect.stringContaining('Start Torrent'))
    })
  })
})
