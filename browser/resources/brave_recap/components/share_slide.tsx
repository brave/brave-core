// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import { getLocale, formatLocale } from '$web-common/locale'
import { RecapStats, getTimeSavedMs } from '../stats'
import { formatBandwidth, formatCount, formatTimeSaved } from '../format'
import { ShareCardImages, renderShareCard } from '../share_card'
import { getShareCardContent } from '../share_content'
import { useMediaQuery } from './hooks'
import { BackButton, ShareCard, SlideLayout, SlideProps } from './slide_layout'
import { LANDSCAPE_QUERY } from './app.style'
import '../strings'

// TODO: replace with an attributed download link once growth provides one.
const DOWNLOAD_URL = 'https://brave.com/download/'

function getShareText(stats: RecapStats) {
  const text = formatLocale(S.BRAVE_RECAP_SHARE_POST_TEXT, {
    $1: formatCount(stats.adsBlocked),
    $2: formatTimeSaved(getTimeSavedMs(stats)).text,
    $3: formatBandwidth(stats.bandwidthSavedBytes).text,
  })
  return `${text} ${DOWNLOAD_URL}`
}

function openShareIntent(url: string) {
  window.open(url, '_blank', 'noopener')
}

export function useShareCard(stats: RecapStats, images: ShareCardImages) {
  const [card, setCard] = React.useState<ShareCard>()
  React.useEffect(() => {
    let url: string | undefined
    let cancelled = false
    renderShareCard(getShareCardContent(stats), images)
      .then((blob) => {
        if (cancelled) {
          return
        }
        url = URL.createObjectURL(blob)
        setCard({ blob, url })
      })
      .catch((e) => {
        console.error('[brave-recap] failed to render share card:', e)
      })
    return () => {
      cancelled = true
      if (url) {
        URL.revokeObjectURL(url)
      }
    }
  }, [stats, images])
  return card
}

// Neither X nor Reddit can take an image through a share link, so the image is
// put on the clipboard to paste into the post.
async function copyImage(card: ShareCard) {
  try {
    await navigator.clipboard.write([
      new ClipboardItem({ [card.blob.type || 'image/png']: card.blob }),
    ])
    return true
  } catch (e) {
    console.error('[brave-recap] failed to copy the share image:', e)
    return false
  }
}

export function ShareSlide({ stats, nav, card }: SlideProps) {
  const [imageCopied, setImageCopied] = React.useState(false)
  // On the wide card the post buttons sit beside the image, off the glass, so
  // they're filled instead of outlined.
  const landscape = useMediaQuery(LANDSCAPE_QUERY)
  const postKind = landscape ? 'filled' : 'outline'
  const postSize = landscape ? 'large' : 'medium'

  const download = () => {
    if (!card) {
      return
    }
    const link = document.createElement('a')
    link.href = card.url
    link.download = 'brave-recap.png'
    link.click()
  }

  // Copy first: the post opens in a new tab, and the clipboard can only be
  // written while this page has focus.
  const share = async (url: string) => {
    if (card && (await copyImage(card))) {
      setImageCopied(true)
    }
    openShareIntent(url)
  }

  const shareText = encodeURIComponent(getShareText(stats))
  const redditTitle = encodeURIComponent(
    formatLocale(S.BRAVE_RECAP_SHARE_POST_TITLE, {
      $1: formatCount(stats.adsBlocked),
    }),
  )

  return (
    <SlideLayout
      start={<BackButton nav={nav} />}
      primary={
        <Button
          kind='filled'
          size='medium'
          onClick={download}
          isDisabled={!card}
        >
          <Icon
            slot='icon-before'
            name='download'
          />
          {getLocale(S.BRAVE_RECAP_DOWNLOAD)}
        </Button>
      }
    >
      <div className='share'>
        <h2 className='visually-hidden'>
          {getLocale(S.BRAVE_RECAP_CARD_TITLE)}
        </h2>
        <div className='share-frame'>
          <div className='card-preview'>
            {card && (
              <img
                src={card.url}
                alt={getLocale(S.BRAVE_RECAP_CARD_TITLE)}
              />
            )}
          </div>
        </div>
        <div className='share-actions'>
          <Button
            kind={postKind}
            size={postSize}
            onClick={() => share(`https://x.com/intent/post?text=${shareText}`)}
          >
            <Icon
              slot='icon-before'
              name='social-x-color'
            />
            {getLocale(S.BRAVE_RECAP_POST_ON_X)}
          </Button>
          <Button
            kind={postKind}
            size={postSize}
            onClick={() =>
              // A text post, so the image can be pasted next to the same text
              // as on X.
              share(
                `https://www.reddit.com/r/brave_browser/submit?selftext=true&title=${redditTitle}&text=${shareText}`,
              )
            }
          >
            <Icon
              slot='icon-before'
              name='social-reddit-color'
            />
            {getLocale(S.BRAVE_RECAP_POST_ON_REDDIT)}
          </Button>
          {imageCopied && (
            <p
              className='share-hint'
              role='status'
            >
              {getLocale(S.BRAVE_RECAP_IMAGE_COPIED)}
            </p>
          )}
        </div>
      </div>
    </SlideLayout>
  )
}
