/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ButtonPrimary, ButtonSecondary, Column, Grid } from 'brave-ui/components'
import { TitleHeading } from 'brave-ui/old'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

// Themes
import { theme } from '../constants/theme'

let clipboardCopy = require('clipboard-copy')

interface Props {
  name?: string | string[]
  torrent?: TorrentObj
  torrentId: string
  tabId: number
  onStartTorrent: (torrentId: string, tabId: number) => void
  onStopDownload: (tabId: number) => void
}

export default class TorrentViewerHeader extends React.PureComponent<Props, {}> {
  constructor (props: Props) {
    super(props)
    this.onClick = this.onClick.bind(this)
    this.onCopyClick = this.onCopyClick.bind(this)
  }

  onClick () {
    this.props.torrent ? this.props.onStopDownload(this.props.tabId)
                       : this.props.onStartTorrent(this.props.torrentId, this.props.tabId)
  }

  onCopyClick () {
    clipboardCopy(this.props.torrentId)
  }

  render () {
    const { name, torrent } = this.props
    const title = name ? 'Start Torrenting "' + name + '"?'
                       : 'Loading torrent information...'
    const mainButtonText = torrent ? 'Stop Download' : 'Start Torrent'

    return (
      <Grid>
        <Column size={9} theme={theme.headerColumnLeft}>
          <TitleHeading
            text={title}
          />
        </Column>
        <Column size={3} theme={theme.headerColumnRight}>
          <ButtonPrimary
            text={mainButtonText}
            color='brand'
            size='medium'
            onClick={this.onClick}
          />
          <ButtonSecondary
            text='Copy Magnet Link'
            color='brand'
            size='medium'
            onClick={this.onCopyClick}
          />
        </Column>
      </Grid>
    )
  }
}
