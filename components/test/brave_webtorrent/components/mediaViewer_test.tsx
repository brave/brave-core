/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { torrentObj } from '../testData'
import MediaViewer from '../../../brave_webtorrent/extension/components/mediaViewer'

describe('mediaViewer component', () => {
  describe('mediaViewer dumb component', () => {
    const ix = 0
    it('renders loading media if not ready', () => {
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentObj}
          ix={ix}
        />
      )
      const assertion = wrapper.find('.loading')
      expect(assertion.length).toBe(1)
    })

    const serverURL = 'http://localhost:12345'
    it('renders video', () => {
      const files = [{ name: 'file.mp4', length: 500 }]
      const torrentWithVideoFile = { ...torrentObj, files, serverURL }
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentWithVideoFile}
          ix={ix}
        />
      )
      const assertion = wrapper.find('#video')
      expect(assertion.length).toBe(1)
    })

    it('renders audio', () => {
      const files = [{ name: 'file.mp3', length: 500 }]
      const torrentWithAudioFile = { ...torrentObj, files, serverURL }
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentWithAudioFile}
          ix={ix}
        />
      )
      const assertion = wrapper.find('#audio')
      expect(assertion.length).toBe(1)
    })

    it('renders img for image', () => {
      const files = [{ name: 'file.jpg', length: 500 }]
      const torrentWithJpgFile = { ...torrentObj, files, serverURL }
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentWithJpgFile}
          ix={ix}
        />
      )
      const assertion = wrapper.find('#image')
      expect(assertion.length).toBe(1)
    })

    it('renders object for PDF file', () => {
      const files = [{ name: 'file.pdf', length: 500 }]
      const torrentWithPdfFile = { ...torrentObj, files, serverURL }
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentWithPdfFile}
          ix={ix}
        />
      )
      const assertion = wrapper.find('#object')
      expect(assertion.length).toBe(1)
    })

    it('renders iframe for text file', () => {
      const files = [{ name: 'file.txt', length: 500 }]
      const torrentWithTxtFile = { ...torrentObj, files, serverURL }
      const wrapper = shallow(
        <MediaViewer
          torrent={torrentWithTxtFile}
          ix={ix}
        />
      )
      const assertion = wrapper.find('#iframe')
      expect(assertion.length).toBe(1)
    })
  })
})
