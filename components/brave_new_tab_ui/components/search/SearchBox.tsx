// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import { getLocale } from '$web-common/locale';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { color, font, spacing } from '@brave/leo/tokens/css/variables';
import * as React from 'react';
import styled from 'styled-components';
import EnginePicker from './EnginePicker';
import { useSearchContext } from './SearchContext';
import { braveSearchHost, searchBoxRadius } from './config';

const searchBoxClass = 'ntp-search-box'

const SearchInput = styled(Input)`
  --leo-control-focus-effect: none;
  --leo-control-padding: 12px 10px;
  --leo-control-text-color: ${color.text.primary};
  --leo-control-font: ${font.large.regular};

  display: inline-block;
  width: 540px;

  leo-icon {
    --leo-icon-color: ${color.icon.default};
  }
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Container = styled.div`
  --leo-control-radius: ${searchBoxRadius};

  display: flex;

  /* If we have search results, don't add a radius to the bottom of the search box */
  &:has(+ .search-results) {
    --leo-control-radius: ${searchBoxRadius} ${searchBoxRadius} 0 0;
  }

  border-radius: var(--leo-control-radius);
`

export default function SearchBox() {
  const { searchEngine, query, setQuery } = useSearchContext()
  const placeholderText = searchEngine?.host === braveSearchHost
    ? getLocale('searchBravePlaceholder')
    : getLocale('searchNonBravePlaceholder')
  const searchInput = React.useRef<HTMLElement>()
  React.useEffect(() => {
    const listener = (e: KeyboardEvent) => {
      if (e.key === 'Tab' && document.activeElement === document.body) {
        e.preventDefault()
        searchInput.current?.focus()
      }
    }
    document.addEventListener('keydown', listener)
    return () => {
      document.removeEventListener('keydown', listener)
    }
  }, [])
  return <Container className={searchBoxClass}>
    <SearchInput tabIndex={1} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.value)} placeholder={placeholderText}>
      <Flex slot="left-icon" align='center'>
        <EnginePicker />
      </Flex>
      <SearchIconContainer slot="right-icon">
        <Icon name="search" />
      </SearchIconContainer>
    </SearchInput>
  </Container>
}
