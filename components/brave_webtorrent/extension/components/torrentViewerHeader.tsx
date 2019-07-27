/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Button, Heading } from 'brave-ui/components'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

let clipboardCopy = require('clipboard-copy')

interface Props {
  name?: string | string[]
  torrent?: TorrentObj
  torrentId: string
  tabId: number
  onStartTorrent: (torrentId: string, tabId: number) => void
  onStopDownload: (tabId: number) => void
}

export default class TorrentViewerHeader extends React.PureComponent<
  Props,
  {}
> {
  onClick = () => {
    this.props.torrent
      ? this.props.onStopDownload(this.props.tabId)
      : this.props.onStartTorrent(this.props.torrentId, this.props.tabId)
  }

  onCopyClick = () => {
    if (this.props.torrentId.startsWith('magnet:')) {
      clipboardCopy(this.props.torrentId)
    } else {
      let a = document.createElement('a')
      a.download = ''
      a.href = this.props.torrentId
      a.click()
    }
  }

  render () {
    const { torrent } = this.props
    const name =
      typeof this.props.name === 'object'
        ? this.props.name[0]
        : this.props.name
    const title = torrent
      ? name
      : name
      ? `${name}`
      : 'Loading torrent information...'
    const mainButtonText = torrent ? 'Stop Torrent' : 'Start Torrent'
    const copyButtonText = this.props.torrentId.startsWith('magnet:')
      ? 'Copy Magnet Link'
      : 'Save .torrent File'

    return (
      <div className='headerContainer'>
        <div className='__column'>
          <Heading children={title} className='__torrentTitle' />
        </div>
        <div className='__column'>
          <Button
            type='accent'
            level={!torrent ? 'primary' : 'secondary'}
            text={mainButtonText}
            onClick={this.onClick}
            className='__button'
          />
          <Button
            type='accent'
            level='secondary'
            text={copyButtonText}
            onClick={this.onCopyClick}
            className='__button'
          />
        </div>
      </div>
    )
  }
}
