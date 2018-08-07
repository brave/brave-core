/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as prettierBytes from 'prettier-bytes'
import { Column, Grid } from 'brave-ui/components'
import { Heading } from 'brave-ui/old'

// Constants
import { TorrentObj } from '../constants/webtorrentState'

interface Props {
  torrent?: TorrentObj
  errorMsg?: string
}

export default class TorrentStatus extends React.PureComponent<Props, {}> {
  render() {
    const { torrent, errorMsg } = this.props

    if (errorMsg) {
      return (
        <div> { errorMsg } </div>
      )
    }

    if (!torrent) {
      return null
    }

    const renderStatus = () => {
      const status = torrent.progress < 1 ? 'Downloading': 'Seeding'
      return (<Column size={2}> { status } </Column>)
    }

    const renderPercentage = () => {
      const percentage = (torrent.progress < 1) ?
        (torrent.progress * 100).toFixed(1) : '100'
      return (<Column size={2}> { percentage } </Column>)
    }

    const renderSpeeds = () => {
      let str = ''
      if (torrent.downloadSpeed > 0) str += ' ↓ ' + prettierBytes(torrent.downloadSpeed) + '/s'
      if (torrent.uploadSpeed > 0) str += ' ↑ ' + prettierBytes(torrent.uploadSpeed) + '/s'
      if (str === '') return
      return (<Column size={2}> { str } </Column>)
    }

    const renderTotalProgress = () => {
      const downloaded = prettierBytes(torrent.downloaded)
      const total = prettierBytes(torrent.length || 0)
      if (downloaded === total) {
        return (<Column size={2}> { downloaded } </Column>)
      } else {
        return (<Column size={2}> { downloaded } / {total} </Column>)
      }
    }

    const renderPeers = () => {
      if (torrent.numPeers === 0) return
      const count = torrent.numPeers === 1 ? 'peer' : 'peers'
      return (<Column size={2}> { torrent.numPeers} { count } </Column>)
    }

    const renderEta = () => {
      if (torrent.timeRemaining === 0 || torrent.timeRemaining === Infinity) return // Zero download speed
      if (torrent.downloaded === torrent.length) return // Already done

      const rawEta = torrent.timeRemaining / 1000
      const hours = Math.floor(rawEta / 3600) % 24
      const minutes = Math.floor(rawEta / 60) % 60
      const seconds = Math.floor(rawEta % 60)

      // Only display hours and minutes if they are greater than 0 but always
      // display minutes if hours is being displayed
      const hoursStr = hours ? hours + 'h' : ''
      const minutesStr = (hours || minutes) ? minutes + 'm' : ''
      const secondsStr = seconds + 's'

      return (<Column size={2}> {hoursStr} {minutesStr} {secondsStr} remaining </Column>)
    }

    return (
      <div>
        <Heading
          text='Torrent Status'
          level={3}
        />
        <Grid>
          { renderStatus() }
          { renderPercentage() }
          { renderSpeeds() }
          { renderTotalProgress() }
          { renderPeers() }
          { renderEta() }
        </Grid>
      </div>
    )
  }
}
