// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { spacing } from '@brave/leo/tokens/css/variables'
import { formatLocale, getLocale } from '$web-common/locale'
import * as React from 'react'
import styled from 'styled-components'
import getBraveNewsController from '../shared/api'
import { ChannelsCachingWrapper } from '../shared/channelsCache'
import { useBraveNews } from '../shared/Context'
import { PublishersCachingWrapper } from '../shared/publishersCache'
import { downloadOpml, importOpml } from './opml'

const Links = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: ${spacing.m};

  & leo-button {
    flex: 0 0 auto;
    white-space: nowrap;
  }
`

// The customize dialog has no toast host, so show the import result as a
// fixed-position alert anchored to the bottom of the (modal) dialog.
const StatusContainer = styled.div`
  position: fixed;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 1;
  max-width: 520px;
`

type Status = { type: 'success' | 'error'; content: string }

export default function OpmlControls() {
  const { publishers, channels, locale } = useBraveNews()
  const inputRef = React.useRef<HTMLInputElement>(null)
  const [status, setStatus] = React.useState<Status | null>(null)

  // Auto-dismiss the status alert.
  React.useEffect(() => {
    if (!status) {
      return
    }
    const timer = setTimeout(() => setStatus(null), 8000)
    return () => clearTimeout(timer)
  }, [status])

  const onExport = () => {
    downloadOpml(Object.values(publishers), Object.values(channels), locale)
  }

  const onFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const input = e.currentTarget
    const file = input.files?.[0]
    // Reset so selecting the same file again re-triggers the change event.
    input.value = ''
    if (!file) {
      return
    }

    try {
      const text = await file.text()
      const result = await importOpml(text, {
        publishers: Object.values(publishers),
        channels: Object.values(channels),
        locale,
        followPublisher: (id) =>
          PublishersCachingWrapper.getInstance().setPublisherFollowed(id, true),
        addDirectFeed: (url) =>
          getBraveNewsController().subscribeToNewDirectFeed({ url }),
        subscribeChannel: (loc, name) =>
          ChannelsCachingWrapper.getInstance().setChannelSubscribed(
            loc,
            name,
            true
          )
      })
      setStatus({
        type: 'success',
        content: formatLocale(S.BRAVE_NEWS_OPML_IMPORT_RESULT, {
          $1: String(result.subscribedPublishers),
          $2: String(result.addedDirectFeeds),
          $3: String(result.subscribedChannels),
          $4: String(result.skipped)
        })
      })
    } catch (err) {
      console.error('Brave News: failed to import OPML', err)
      setStatus({
        type: 'error',
        content: getLocale(S.BRAVE_NEWS_OPML_IMPORT_ERROR)
      })
    }
  }

  return (
    <>
      <Links>
        <Button kind='plain-faint' size='small' onClick={() => inputRef.current?.click()}>
          {getLocale(S.BRAVE_NEWS_OPML_IMPORT)}
        </Button>
        <Button kind='plain-faint' size='small' onClick={onExport}>
          {getLocale(S.BRAVE_NEWS_OPML_EXPORT)}
        </Button>
      </Links>
      <input
        ref={inputRef}
        type='file'
        accept='.opml,.xml,text/xml,application/xml,text/x-opml'
        hidden
        onChange={onFileChange}
      />
      {status && (
        <StatusContainer>
          <Alert type={status.type}>{status.content}</Alert>
        </StatusContainer>
      )}
    </>
  )
}
