// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color, font, radius, spacing } from '@brave/leo/tokens/css';
import { mojoString16ToString } from 'chrome://resources/js/mojo_type_util.js';
import { AutocompleteMatch } from 'gen/components/omnibox/browser/omnibox.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import Flex from '../../../common/Flex';
import { omniboxController } from './SearchBox';

interface Props {
  match: AutocompleteMatch
  line: number
  selected: boolean
}

const Container = styled.a`
  padding: ${spacing.s} ${spacing['2Xl']};

  display: flex;
  flex-direction: row;
  align-items: center;
  gap: ${spacing.l};

  text-decoration: none;

  &[aria-selected=true] {
    background: ${color.container.interactive};
  }

  &:hover {
    background: ${color.container.highlight};
  }
`

const IconContainer = styled.div`
  border-radius: ${radius.m};
  width: 32px;
  height: 32px;

  background: ${color.container.highlight};

  display: flex;
  align-items: center;
  justify-content: center;

  flex-shrink: 0;
`

const FavIcon = styled.img`
  width: 20px;
  height: 20px;
`

const Content = styled.span`
  font: ${font.large.regular};
  color: ${color.text.secondary};
`

const Description = styled.span`
  font: ${font.small.regular};
  color: ${color.text.tertiary};
`

const Hint = styled.span`
  color: ${color.text.interactive};
`

export default function SearchResult({ match, line, selected }: Props) {
  const contents = mojoString16ToString(match.swapContentsAndDescription ? match.description : match.contents)
  const description = mojoString16ToString(match.swapContentsAndDescription ? match.contents : match.description)

  const hint = description && match.destinationUrl.url
    ? description
    : ''

  const subtitle = match.destinationUrl.url || description
  return <Container href={match.destinationUrl.url} aria-selected={selected} onClick={e => {
    e.preventDefault()
    omniboxController.openAutocompleteMatch(line, match.destinationUrl, true, e.button, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey)
  }}>
    <IconContainer>
      <FavIcon src={match.iconUrl} />
    </IconContainer>
    <Flex direction='column'>
      <Content>{contents}<Hint>{hint ? ` - ${hint}` : ''}</Hint></Content>
      <Description>{subtitle}</Description>
    </Flex>
  </Container>
}
