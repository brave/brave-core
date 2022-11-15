/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Button } from 'brave-ui/components'
import * as React from 'react'

// Constants
import styled from 'styled-components'
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

const HeaderText = styled.h1`
  all: unset;
  font-weight: 600;
  font-size: 28px;
  line-height: 38px;
`

const HeaderRow = styled.div`
  display: flex;
  justify-content: space-between;
`

const ButtonsContainer = styled.div`
  display: flex;
  gap: 8px;
  --button-main-color: var(--interactive2);
`

export default function TorrentHeaderViewer ({ torrent, torrentId, name, tabId, onStartTorrent, onStopDownload }: Props) {
  name = typeof name === 'object'
      ? name[0]
      : name
  const title = name || 'Loading torrent information...'
  const mainButtonText = torrent ? 'Stop Torrent' : 'Start Torrent'
  const copyButtonText = torrentId.startsWith('magnet:')
    ? 'Copy Magnet Link'
    : 'Save .torrent File'

  const onCopyClick = async () => {
    if (torrentId.startsWith('magnet:')) {
      try {
        await navigator.clipboard.writeText(torrentId)
        console.log('Copy succeeded')
      } catch (e) {
        console.log('Copy failed')
      }
    } else {
      const downloadListener = (item: chrome.downloads.DownloadItem) => {
        chrome.downloads.onDeterminingFilename.removeListener(downloadListener)

        if (!item || !item.url || item.url !== torrentId) {
          // Only listen for downloads initiated by Webtorrent.
          return
        }
        const url = new URL(item.finalUrl || item.url)
        if (!url || !remoteProtocols.includes(url.protocol) ||
            url.hostname === '127.0.0.1') {
          // Non-remote files are trusted.
          return
        }
        if (!item.filename || !item.filename.endsWith('.torrent')) {
          try {
            chrome.downloads.cancel(item.id, () => {
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
            })
          } catch (e) {
            console.error(e)
          }
        }
      }

      // Listen for malicious files pretending to be .torrents
      chrome.downloads.onDeterminingFilename.addListener(downloadListener)
      const a = document.createElement('a')
      a.download = ''
      a.href = torrentId
      a.click()
    }
  }

  return <HeaderRow>
    <HeaderText>{title}</HeaderText>
    <ButtonsContainer>
      <Button
        type='accent'
        level={!torrent ? 'primary' : 'secondary'}
        text={mainButtonText}
        onClick={() => torrent ? onStopDownload(tabId) : onStartTorrent(torrentId, tabId)} />
      <Button
        type='accent'
        level='secondary'
        text={copyButtonText}
        onClick={onCopyClick} />
    </ButtonsContainer>
  </HeaderRow>
}
