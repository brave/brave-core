// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color, font, gradient, icon, radius, spacing } from '@brave/leo/tokens/css/variables';
import { mojoString16ToString } from 'chrome://resources/js/mojo_type_util.js';
import { AutocompleteMatch } from 'gen/ui/webui/resources/cr_components/searchbox/searchbox.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import Flex from '$web-common/Flex';
import { omniboxController } from './SearchContext';
import { getLocale } from '$web-common/locale';
import Icon from '@brave/leo/react/icon';

interface Props {
  match: AutocompleteMatch
  line: number
  selected: boolean
}

const Container = styled.a`
  padding: ${spacing.s} ${spacing.xl};
  border-radius: ${radius.m};

  display: flex;
  flex-direction: row;
  align-items: center;
  gap: ${spacing.xl};

  text-decoration: none;

  &[aria-selected=true], &:hover {
    background: rgba(255, 255, 255, 0.1);
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
`

const FavIcon = styled.span<{ url: string }>`
  width: 20px;
  height: 20px;
  background: rgba(255, 255, 255, 0.5);
  mask-image: url(${p => p.url});
  mask-size: contain;
`

const Content = styled.span`
  font: ${font.large.regular};
  color: ${color.text.secondary};
`

const Description = styled.span`
  font: ${font.small.regular};
  color: rgba(255,255,255,0.7);
`

const Hint = styled.span`
  color: ${color.text.interactive};
`

const LeoIcon = styled(Icon)`
  --leo-icon-size: ${icon.m};

  color: ${color.white};
  background: ${gradient.iconsActive};
  border-radius: ${radius.m};
  padding: ${spacing.s};
`

const Divider = styled.hr`
  margin: -2px -8px;
  opacity: 0.1;
`

export default function SearchResult({ match, line, selected }: Props) {
  const contents = mojoString16ToString(match.swapContentsAndDescription ? match.description : match.contents)
  const description = mojoString16ToString(match.swapContentsAndDescription ? match.contents : match.description)
  const isAskLeo = description === getLocale('searchAskLeo')

  const hint = description && match.destinationUrl.url
    ? description
    : ''

  const subtitle = match.destinationUrl.url || description
  const result = <Container href={match.destinationUrl.url} aria-selected={selected} onClick={e => {
    e.preventDefault()
    omniboxController.openAutocompleteMatch(line, match.destinationUrl, true, e.button, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey)
  }}>
    <IconContainer>
      {isAskLeo
        ? <LeoIcon name="product-brave-leo" />
        : <FavIcon url={match.iconUrl} />}
    </IconContainer>
    <Flex direction='column'>
      <Content>{contents}<Hint>{hint ? ` - ${hint}` : ''}</Hint></Content>
      <Description>{subtitle}</Description>
    </Flex>
  </Container>

  return isAskLeo
    ? <>
      <Divider />
      {result}
    </>
    : result
}
