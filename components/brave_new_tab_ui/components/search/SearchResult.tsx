// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import { getLocale } from '$web-common/locale';
import Icon from '@brave/leo/react/icon';
import { color, font, gradient, icon, radius, spacing } from '@brave/leo/tokens/css/variables';
import { mojoString16ToString } from 'chrome://resources/js/mojo_type_util.js';
import { AutocompleteMatch } from 'gen/ui/webui/resources/cr_components/searchbox/searchbox.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import { useUnpaddedImageUrl } from '../../../brave_news/browser/resources/shared/useUnpaddedImageUrl';

interface Props {
  match: AutocompleteMatch
  onClick: (e: React.MouseEvent) => void
  selected: boolean
}

const Container = styled.a`
  padding: ${spacing.s} ${spacing.m};
  border-radius: ${radius.m};

  display: flex;
  flex-direction: row;
  align-items: center;
  gap: ${spacing.l};

  text-decoration: none;

  overflow: hidden;

  &[aria-selected=true], &:hover {
    background: color-mix(in srgb, ${color.text.primary} 10%, transparent 90%);
  }
`

const IconContainer = styled.div`
  border-radius: ${radius.m};
  width: 32px;
  height: 32px;

  display: flex;
  align-items: center;
  justify-content: center;

  flex-shrink: 0;

  > * { border-radius: ${radius.s}; }

  > span {
    width: 20px;
    height: 20px;
  }

  > img {
    width: 100%;
    height: 100%;
  }
`

const SearchIcon = styled.span<{ url: string }>`
  background: color-mix(in srgb, ${color.icon.default} 50%, transparent 50%);
  mask-image: url(${p => p.url});
  mask-size: contain;
`

const FavIcon = styled.span<{ url: string }>`
  background-image: url(${p => p.url});
  background-size: contain;
`

const Content = styled.span`
  font: ${font.large.regular};
  line-height: 24px;
  color: ${color.text.primary};
`

const Description = styled.span`
  font: ${font.small.regular};
  line-height: 18px;
  color: ${color.text.secondary};
`

const LeoIcon = styled(Icon)`
  --leo-icon-size: ${icon.l};

  color: ${color.white};
  background: ${gradient.iconsActive};
  border-radius: ${radius.m};
  padding: ${spacing.s};
`

const Divider = styled.hr`
  margin: 2px -8px;
  opacity: 0.1;
`

const hide = { opacity: 0 }
const show = { opacity: 1 }
function RichImage({ url }: { url: string }) {
  const [loaded, setLoaded] = React.useState(false)
  const iconUrl = useUnpaddedImageUrl(url, () => setLoaded(true))
  return <img src={iconUrl} style={loaded ? show : hide} />
}
function Image({ match, isAskLeo }: { match: AutocompleteMatch, isAskLeo: boolean }) {
  // AskLeo is a case we treat specially. It's included on most queries.
  if (isAskLeo) return <LeoIcon name='product-brave-leo' />

  // We have three separate cases here:
  // 1. A chromium generic search result icon:
  //    We display the icon as a mask-image, so we can change the color
  // 2. An `imageUrl` with the chrome:// scheme, which we can load on the NTP:
  //    We display the image as a background, as its safe on the NTP
  // 3. A web resource, which we need to load from the WebUI via the unpadded
  //    url machinery.
  const isGeneric = !match.imageUrl
  return isGeneric
    ? <SearchIcon url={match.iconUrl} />
    : match.imageUrl.startsWith('chrome')
      ? <FavIcon url={match.imageUrl} />
      : <RichImage url={match.imageUrl} />
}

export default function SearchResult({ match, selected, onClick }: Props) {
  const contents = mojoString16ToString(match.swapContentsAndDescription ? match.description : match.contents)
  const description = mojoString16ToString(match.swapContentsAndDescription ? match.contents : match.description)
  const isAskLeo = description === getLocale('searchAskLeo')

  const result = <Container href={match.destinationUrl.url} aria-selected={selected} onClick={e => {
    e.preventDefault()
    onClick(e)
  }}>
    <IconContainer>
      <Image key={match.imageUrl ?? match.iconUrl} match={match} isAskLeo={isAskLeo} />
    </IconContainer>
    <Flex direction='column'>
      <Content>{contents}</Content>
      {description && <Description>{description}</Description>}
    </Flex>
  </Container>

  return isAskLeo
    ? <>
      <Divider />
      {result}
    </>
    : result
}
