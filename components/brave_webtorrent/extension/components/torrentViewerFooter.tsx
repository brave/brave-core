/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Anchor, Heading, Paragraph } from 'brave-ui/old'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

// Themes
import { theme } from '../constants/theme'

interface Props {
  torrent?: TorrentObj
}

export default class TorrentViewerFooter extends React.PureComponent<Props, {}> {
  render () {
    const { torrent } = this.props
    const privacyNotice = 'When you click "Start Torrent", Brave will download pieces of the torrent file from other users and upload pieces to them in turn. This will share the fact that you\'re downloading this file: other people will know what you\'re downloading, and they may also have access to your public IP address.'

    return (
      torrent ?
      (
        <Anchor
          href='https://webtorrent.io'
          text='Powered By WebTorrent'
          target='_blank'
        />
      )
      :
      (
        <div className='privacy-warning'>
          <Heading
            text='Privacy Warning:'
            level={4}
          />
        <Paragraph
          text={privacyNotice}
          theme={theme.privacyNoticeBody}
        />
        </div>
      )
    )
  }
}
