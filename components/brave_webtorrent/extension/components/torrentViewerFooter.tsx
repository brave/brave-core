/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Anchor } from 'brave-ui/old'
import * as React from 'react'

// Constants
import styled from 'styled-components'
import { TorrentObj } from '../constants/webtorrentState'

interface Props {
  torrent?: TorrentObj
}

const FooterNotice = styled.div`
  padding: 18px 0;
  color: var(--text3);
`

export default function TorrentViewerFooter ({ torrent }: Props) {
  return torrent
      ? <Anchor
        href='https://webtorrent.io'
        text='Powered By WebTorrent'
        target='_blank' />
      : <FooterNotice>
          <b>Privacy Warning:</b> When you click "Start Torrent" Brave will begin
          downloading pieces of the torrent file from other users and uploading to
          them in turn. This action will share that you're downloading this file.
          Others may be able to see what you're downloading and/or determine your
          public IP address. The download may bypass your proxy settings.
          <br /><br />
          The WebTorrent extension can be disabled from Brave settings.
    </FooterNotice>
}
