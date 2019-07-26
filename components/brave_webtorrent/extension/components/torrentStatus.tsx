/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as prettierBytes from 'prettier-bytes'
import { Heading } from 'brave-ui/components'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

const TorrentStatDivider = () => (
  <span className='torrentStatDivider'>•</span>
 )

interface Props {
  torrent?: TorrentObj
  errorMsg?: string
}

export default class TorrentStatus extends React.PureComponent<Props, {}> {
  render () {
    const { torrent, errorMsg } = this.props

    if (errorMsg) {
      return <div> {errorMsg} </div>
    }

    if (!torrent) {
      return null
    }

    const renderStatus = () => {
      const status = torrent.progress < 1
        ? 'Downloading'
        : 'Seeding'
      return <span className='torrentStat'>{status}</span>
    }

    const renderPercentage = () => {
      const percentage = torrent.progress < 1
        ? (torrent.progress * 100).toFixed(1)
        : '100'
      return (
        <>
          <TorrentStatDivider />
          <span className='torrentStat'>{percentage}%</span>
        </>
      )
    }

    const renderDownloadSpeed = () => {
      if (torrent.downloadSpeed === 0) return
      return (
        <>
          <TorrentStatDivider />
          <span className='torrentStat'>
            ↓ {prettierBytes(torrent.downloadSpeed)}/s
          </span>
        </>
      )
    }

    const renderUploadSpeed = () => {
      if (torrent.uploadSpeed === 0) return
      return (
        <>
          <TorrentStatDivider />
          <span className='torrentStat'>
            ↑ {prettierBytes(torrent.uploadSpeed)}/s
          </span>
        </>
      )
    }

    const renderTotalProgress = () => {
      const downloaded = prettierBytes(torrent.downloaded)
      const total = prettierBytes(torrent.length || 0)
      if (downloaded === total) {
        return (
          <>
            <TorrentStatDivider />
            <span className='torrentStat'>{downloaded}</span>
          </>
        )
      } else {
        return (
          <>
            <TorrentStatDivider />
            <span className='torrentStat'>{downloaded} / {total}</span>
          </>
        )
      }
    }

    const renderPeers = () => {
      if (torrent.numPeers === 0) return
      const count = torrent.numPeers === 1 ? 'peer' : 'peers'
      return (
        <>
          <TorrentStatDivider />
          <span className='torrentStat'>
            {torrent.numPeers} {count}
          </span>
        </>
      )
    }

    const renderEta = () => {
      if (torrent.timeRemaining === 0 || torrent.timeRemaining === Infinity) {
        return
      } // Zero download speed
      if (torrent.downloaded === torrent.length) return // Already done

      const rawEta = torrent.timeRemaining / 1000
      const hours = Math.floor(rawEta / 3600) % 24
      const minutes = Math.floor(rawEta / 60) % 60
      const seconds = Math.floor(rawEta % 60)

      // Only display hours and minutes if they are greater than 0 but always
      // display minutes if hours is being displayed
      const hoursStr = hours ? hours + 'h' : ''
      const minutesStr = hours || minutes ? minutes + 'm' : ''
      const secondsStr = seconds + 's'

      return (
        <>
          <TorrentStatDivider />
          <span className='torrentStat'>
            {hoursStr} {minutesStr} {secondsStr} remaining
          </span>
        </>
      )
    }

    return (
      <div className='torrentSubhead'>
        <Heading children='Torrent Stats' level={2} className='torrentHeading' />
        <div className='torrentStatus'>
          {renderStatus()}
          {renderPercentage()}
          {renderDownloadSpeed()}
          {renderUploadSpeed()}
          {renderTotalProgress()}
          {renderPeers()}
          {renderEta()}
        </div>
      </div>
    )
  }
}
