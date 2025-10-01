// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css/variables';
import styled from "styled-components";
import SecureLink, { SecureLinkProps, defaultAllowedSchemes, validateScheme } from '$web-common/SecureLink';
import { configurationCache, useBraveNews } from '../shared/Context';

export const Header = styled.h2`
  margin: 0;

  font: ${font.heading.h2};
  color: var(--bn-glass-100);

  --leo-icon-size: 18px;
`

export const Title = styled.h3`
  --leo-icon-color: ${color.icon.default};

  display: flex;
  align-items: center;

  margin: 0;

  text-align: start;
  font: ${font.default.semibold};
  color: var(--bn-glass-100);


  &> a { all: unset; }
`

const BaseImg = styled.img`
  opacity: 0;
  transition: opacity 400ms ease-in-out;
`

type HidabeImageProps = React.DetailedHTMLProps<
  React.ImgHTMLAttributes<HTMLImageElement>,
  HTMLImageElement
> & {
  targetHeight?: number
  targetWidth?: number
}
const HidableImage = ({
  onError,
  onLoad,
  targetHeight,
  targetWidth,
  src,
  loading,
  ...rest
}: HidabeImageProps) => {
  const ref = React.useRef<HTMLImageElement>()

  const { shouldRenderImages } = useBraveNews()
  const delayLoad = !shouldRenderImages && loading === 'lazy'

  const handleLoad = React.useCallback((e: React.UIEvent<HTMLImageElement>) => {
    ref.current!.style.opacity = '1'
    onLoad?.(e)
  }, [onLoad])

  if (src) {
    src = 'chrome://brave-image?url=' + encodeURIComponent(src);
  }
  if (targetHeight && targetWidth) {
    const width = Math.round(targetWidth * window.devicePixelRatio)
    const height = Math.round(targetHeight * window.devicePixelRatio)
    src += `&target_size=${width}x${height}`
  }

  return <BaseImg
    {...rest}
    src={delayLoad ? undefined : src}
    loading={loading}
    ref={ref as any}
    onLoad={handleLoad}
  />
}

export const SmallImage = styled(HidableImage).attrs({ targetHeight: 64, targetWidth: 96 })`
  &:not([src]) { opacity: 0; }

  min-width: 96px;
  width: 96px;

  height: 64px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export const LargeImage = styled(HidableImage).attrs({ targetHeight: 269, targetWidth: 508 })`
  &:not([src]) { opacity: 0; }

  width: 100%;
  height: 269px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export default styled.div`
  text-decoration: none;
  background: var(--bn-glass-container);
  border-radius: ${radius.xl};
  color: var(--bn-glass-100);
  padding: ${spacing.xl};

  &:has(${Title} a:focus-visible) {
    box-shadow: ${effect.focusState};
  }

  ${p => p.onClick && 'cursor: pointer'}
`

export const braveNewsCardClickHandler = (href: string | undefined, allowedSchemes: string[] = defaultAllowedSchemes) => (e: React.MouseEvent) => {
  validateScheme(href, allowedSchemes)

  if (configurationCache.value.openArticlesInNewTab || e.ctrlKey || e.metaKey || e.buttons & 4) {
    window.open(href, '_blank', 'noopener noreferrer')
  } else {
    window.location.href = href!
  }
}

interface BraveNewsLinkProps extends SecureLinkProps {
  feedDepth?: number
}

export function BraveNewsLink({ feedDepth, ...rest }: BraveNewsLinkProps) {
  const props = { feeddepth: feedDepth, ...rest }
  const { openArticlesInNewTab, reportVisit } = useBraveNews()
  return <SecureLink
    {...props}
    onClick={e => {
      e.stopPropagation()
      if (feedDepth !== undefined) {
        reportVisit(feedDepth);
      }
    }}
    target={openArticlesInNewTab ? '_blank' : undefined}
  />
}
