// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import { getLocale } from '$web-common/locale';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables';
import * as React from 'react';
import styled from 'styled-components';
import { useSearchContext } from './SearchContext';
import { braveSearchHost } from './config';
import EnginePicker from './EnginePicker';

const SearchInput = styled(Input)`
  --leo-control-focus-effect: none;
  --leo-control-padding: 6px;
  --leo-control-color: rgba(255, 255, 255, 0.1);
  --leo-control-text-color: ${color.white};
  --leo-control-font: ${font.large.regular};

  display: inline-block;
  width: 540px;

  leo-icon {
    --leo-icon-color: rgba(255, 255, 255, 0.5);
  }
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Container = styled.div`
  --leo-control-radius: ${radius.m};

  display: flex;

  /* If we have search results, don't add a radius to the bottom of the search box */
  &:has(+ .search-results) {
    --leo-control-radius: ${radius.m} ${radius.m} 0 0;
  }

  border-radius: var(--leo-control-radius);
`

export const Backdrop = styled.div`
  z-index: -1;
  position: absolute;
  inset: 0;
  backdrop-filter: blur(64px);
  border-radius: ${radius.m};
`

export default function SearchBox() {
  const { searchEngine, query, setQuery } = useSearchContext()
  const placeholderText = searchEngine?.host === braveSearchHost
    ? getLocale('searchBravePlaceholder')
    : getLocale('searchNonBravePlaceholder')
  const searchInput = React.useRef<HTMLElement>()
  return <Container>
    <SearchInput tabIndex={0} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.value)} placeholder={placeholderText}>
      <Flex slot="left-icon">
        <EnginePicker />
      </Flex>
      <SearchIconContainer slot="right-icon">
        <Icon name="search" />
      </SearchIconContainer>
    </SearchInput>
  </Container>
}
