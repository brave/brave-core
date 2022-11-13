/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as prettierBytes from 'prettier-bytes'
import * as React from 'react'

// Constants
import styled from 'styled-components'
import { TorrentObj } from '../constants/webtorrentState'
import { Header } from './header'

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: 16px;
`

const StatsContainer = styled.div`
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  
  padding: 16px;
  border-radius: 8px;

  background: var(--background1);

  box-shadow: 0px 0.5px 1.5px 0px rgb(0 0 0 / 15%);

  font-size: 16px;
  font-weight: 400;
  line-height: 24px;
`

const StatusText = styled.span`
  font-weight: 600;
`

const StatsDivider = () => <span>•</span>

interface Props {
  torrent?: TorrentObj
  errorMsg?: string
}

function Eta ({ torrent }: {torrent: TorrentObj}) {
   // Zero download speed
  if (torrent.timeRemaining === 0 || torrent.timeRemaining === Infinity) return null

  // Already done
  if (torrent.downloaded === torrent.length) return null

  const rawEta = torrent.timeRemaining / 1000
  const hours = Math.floor(rawEta / 3600) % 24
  const minutes = Math.floor(rawEta / 60) % 60
  const seconds = Math.floor(rawEta % 60)

  // Only display hours and minutes if they are greater than 0 but always
  // display minutes if hours is being displayed
  const hoursStr = hours ? hours + 'h' : ''
  const minutesStr = hours || minutes ? minutes + 'm' : ''
  const secondsStr = seconds + 's'

  return <>
      <StatsDivider />
      <span>
        {hoursStr} {minutesStr} {secondsStr} remaining
      </span>
    </>
}

export default function TorrentStatus ({ torrent, errorMsg }: Props) {
  if (errorMsg) return <div>{errorMsg}</div>
  if (!torrent) return null

  const downloaded = prettierBytes(torrent.downloaded)
  const total = prettierBytes(torrent.length)
  return <Container>
    <Header>Torrent stats</Header>
    <StatsContainer>
      <StatusText>{torrent.progress < 1 ? 'Downloading' : 'Seeding'}</StatusText>
      <StatsDivider/>
      <span>{torrent.progress < 1 ? (torrent.progress * 100).toFixed(1) : 100}%</span>
      <StatsDivider/>
      {torrent.downloadSpeed !== 0 && <>
        <span>↓ {prettierBytes(torrent.downloadSpeed)}/s</span>
        <StatsDivider/>
      </>}
      {torrent.uploadSpeed !== 0 && <>
        <span>↑ {prettierBytes(torrent.uploadSpeed)}/s</span>
        <StatsDivider/>
      </>}
      <span>{downloaded}{downloaded !== total && ` / ${total}`}</span>
      <StatsDivider/>
      <span>{torrent.numPeers} {torrent.numPeers === 1 ? 'peer' : 'peers'}</span>
      <Eta torrent={torrent}/>
    </StatsContainer>
  </Container>
}
