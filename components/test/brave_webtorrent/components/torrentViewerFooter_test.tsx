/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import TorrentViewerFooter from '../../../brave_webtorrent/extension/components/torrentViewerFooter'

describe('torrentViewerFooter component', () => {
  describe('torrentViewerFooter dumb component', () => {
    it('renders the component with webtorrent link if torrent is passed', () => {
      const wrapper = shallow(
        <TorrentViewerFooter
          torrent={torrentObj}
        />
      )
      const assertion = wrapper.find({ href: 'https://webtorrent.io' })
      expect(assertion.length).toBe(1)
    })

    it('renders the component with privacy notice if no torrent object', () => {
      const wrapper = shallow(
        <TorrentViewerFooter />
      )
      const assertion = wrapper.find('.footerNotice')
      expect(assertion.length).toBe(1)
    })
  })
})
