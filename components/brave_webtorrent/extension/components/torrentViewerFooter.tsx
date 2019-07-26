/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Anchor } from 'brave-ui/old'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

// Themes
// import { theme } from '../constants/theme'

interface Props {
  torrent?: TorrentObj
}

export default class TorrentViewerFooter extends React.PureComponent<
  Props,
  {}
> {
  render () {
    const { torrent } = this.props

    if (torrent) {
      return (
        <Anchor
          href='https://webtorrent.io'
          text='Powered By WebTorrent'
          target='_blank'
          id='webTorrentCredit'
        />
      )
    } else {
      return (
        <div className='footerNotice'>
          Privacy Warning: When you click "Start Torrent" Brave will begin
          downloading pieces of the torrent file from other users and uploading to
          them in turn. This action will share that you're downloading this file.
          Others may be able to see what you're downloading and/or determine your
          public IP address.
          <br /><br />
          The WebTorrent extension can be disabled from Brave settings.
        </div>
      )
    }
  }
}
