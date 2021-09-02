/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Button, Heading } from 'brave-ui/components'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

interface Props {
  name?: string | string[]
  torrent?: TorrentObj
  torrentId: string
  tabId: number
  onStartTorrent: (torrentId: string, tabId: number) => void
  onStopDownload: (tabId: number) => void
}

const remoteProtocols = ['http:', 'https:', 'ftp:', 'sftp:', 'ws:', 'wss:']

export default class TorrentViewerHeader extends React.PureComponent<
  Props,
  {}
> {
  onClick = () => {
    this.props.torrent
      ? this.props.onStopDownload(this.props.tabId)
      : this.props.onStartTorrent(this.props.torrentId, this.props.tabId)
  }

  removeDownloadListener = () => {
    chrome.downloads.onDeterminingFilename.removeListener(this.downloadListener)
  }

  downloadListener = (item: chrome.downloads.DownloadItem) => {
    if (!item || !item.url || item.url !== this.props.torrentId) {
      // Only listen for downloads initiated by Webtorrent.
      this.removeDownloadListener()
      return
    }
    const url = new URL(item.finalUrl || item.url)
    if (!url || !remoteProtocols.includes(url.protocol) ||
        url.hostname === '127.0.0.1') {
      // Non-remote files are trusted.
      this.removeDownloadListener()
      return
    }
    if (!item.filename || !item.filename.endsWith('.torrent')) {
      try {
        chrome.downloads.cancel(item.id, this.onTorrentDownloadError)
      } catch (e) {
        this.removeDownloadListener()
        console.error(e)
      }
    }
  }

  onTorrentDownloadError = () => {
    this.removeDownloadListener()
    const msg = 'Error: file is not a .torrent.'
    // By default, alert() shows up on every tab that has loaded webtorrent.
    // We only want it to show on the current tab.
    chrome.tabs.query({
      active: true,
      lastFocusedWindow: true
    }, (tabs) => {
      const tab = tabs[0]
      if (tab && typeof tab.id === 'number') {
        chrome.tabs.executeScript(tab.id, {
          code: `alert('${msg}')`
        }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.executeScript failed: ' + chrome.runtime.lastError.message)
          }
        })
        return
      }
      alert(msg)
    })
  }

  onCopyClick = async () => {
    if (this.props.torrentId.startsWith('magnet:')) {
      try {
        await navigator.clipboard.writeText(this.props.torrentId)
        console.log('Copy succeeded')
      } catch (e) {
        console.log('Copy failed')
      }
    } else {
      // Listen for malicious files pretending to be .torrents (#11488)
      chrome.downloads.onDeterminingFilename.addListener(this.downloadListener)
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
