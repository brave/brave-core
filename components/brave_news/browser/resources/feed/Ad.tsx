// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import SecureLink, { validateScheme } from '$web-common/SecureLink';
import { useOnVisibleCallback } from '$web-common/useVisible';
import VisibilityTimer from '$web-common/visibilityTimer';
import Button from '@brave/leo/react/button';
import { font, spacing } from '@brave/leo/tokens/css';
import { DisplayAd, FeedV2Ad } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import getBraveNewsController from '../shared/api';
import { useUnpaddedImageUrl } from '../shared/useUnpaddedImageUrl';
import { MetaInfoContainer } from './ArticleMetaRow';
import Card, { LargeImage, Title } from './Card';

interface Props {
  info: FeedV2Ad
}

const Container = styled(Card)`
  display: flex;
  flex-direction: column;
  gap: ${spacing.m}
`

const BatAdLabel = styled.a`
  padding: 0 2px;

  border: 1px solid rgba(var(--bn-text-base), 0.3);
  border-radius: 3px;

  text-decoration: none;

  color: rgba(var(--bn-text-base, 0.7));
  font: ${font.primary.small.regular};
`

const CtaButton = styled(Button)`
  align-self: flex-start;
`

const openLink = (link: string) => {
  validateScheme(link)
  window.location.href = link
}

export const useVisibleFor = (callback: () => void, timeout: number) => {
  const [el, setEl] = React.useState<HTMLElement | null>(null)
  const callbackRef = React.useRef<() => void>()
  callbackRef.current = callback

  React.useEffect(() => {
    if (!el) return

    const observer = new VisibilityTimer(() => {
      callbackRef.current?.()
    }, timeout, el)
    observer.startTracking()

    return () => {
      observer.stopTracking()
    }
  }, [el, timeout, callback])

  return {
    setEl,
  }
}

export default function Advert(props: Props) {
  const [advert, setAdvert] = React.useState<DisplayAd | null | undefined>(undefined)
  const imageUrl = useUnpaddedImageUrl(advert?.image.paddedImageUrl?.url ?? advert?.image.imageUrl?.url)

  const onDisplayAdViewed = React.useCallback(() => {
    if (!advert) return

    console.debug(`Brave News: Viewed display ad: ${advert.uuid}`)
    getBraveNewsController().onDisplayAdView(advert.uuid, advert.creativeInstanceId)
  }, [advert])

  const { setEl: setAdEl } = useVisibleFor(onDisplayAdViewed, 1000)

  const onDisplayAdVisited = React.useCallback(async () => {
    if (!advert) return

    console.debug(`Brave News: Visited display ad: ${advert.uuid}`)
    await getBraveNewsController().onDisplayAdVisit(advert.uuid, advert.creativeInstanceId)
    openLink(advert.targetUrl.url)
  }, [advert])

  const { setElementRef: setTriggerRef } = useOnVisibleCallback(async () => {
    console.debug(`Brave News: Fetching an advertisement`)

    const advert = await getBraveNewsController().getDisplayAd().then(r => r.ad) ?? {
      title: '10 reasons why technica recreated the sound of old classics.',
      description: 'Technica',
      creativeInstanceId: '1234',
      ctaText: 'Get Started',
      targetUrl: { url: 'https://www.brave.com' },
      image: { imageUrl: undefined, paddedImageUrl: { url: 'https://pcdn.bravesoftware.com/brave-today/favicons/f35d56d075b3015a7eef41273d56f4f9665b3749b2318abf531a20722b824bb3.jpg.pad' } },
      dimensions: '1x3',
      uuid: '0abc'
    }

    // TODO(fallaciousreasoning): Set null if no response comes back
    // (at the moment I'm injecting fake display ads instead).
    setAdvert(advert)
  }, {
    // Trigger ad fetch when the ad unit is 1000px away from the viewport
    rootMargin: '0px 0px 1000px 0px'
  })

  // Advert is null if we didn't manage to load an advertisement
  if (advert === null) return null

  // Otherwise, render a placeholder div - when close to the viewport we'll
  // request an ad.
  if (!advert) {
    return <div ref={setTriggerRef} />
  }

  return <Container ref={setAdEl} onClick={onDisplayAdVisited}>
    <LargeImage src={imageUrl} />
    <MetaInfoContainer>
      <BatAdLabel onClick={e => e.stopPropagation()} href="brave://rewards">Ad</BatAdLabel>
      •
      {' ' + advert.description}
    </MetaInfoContainer>
    <Title>
      <SecureLink href={advert.targetUrl.url} onClick={e => {
        // preventDefault, so we go through onDisplayAdVisit and record the
        // result.
        e.preventDefault()
      }}>
        {advert.title}
      </SecureLink>
    </Title>
    <CtaButton kind='outline'>{advert.ctaText}</CtaButton>
  </Container>
}
