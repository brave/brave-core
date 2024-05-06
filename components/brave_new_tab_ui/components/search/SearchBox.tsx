// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import Dropdown from '@brave/leo/react/dropdown';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { color, radius, spacing } from '@brave/leo/tokens/css/variables';
import * as React from 'react';
import styled from 'styled-components';
import { MediumIcon } from './SearchEngineIcon';
import { useSearchContext } from './SearchContext';
import { braveSearchHost } from './config';
import Button from '@brave/leo/react/button';

const SearchInput = styled(Input)`
  --leo-control-focus-effect: none;
  --leo-control-padding: 6px;
  --leo-control-color: rgba(255, 255, 255, 0.1);

  display: inline-block;
  width: 540px;
`

const EnginePicker = styled(Dropdown)`
  --leo-control-radius: ${radius.m};
`

const EngineValueSlot = styled.div`
  display: flex;
  padding: 2px 0;
  margin: 0 -4px;
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Option = styled.div`
    display: flex;
    gap: ${spacing.m};
`

const CustomizeButton = styled(Button)`
  border-top: 1px solid ${color.divider.subtle};
  color: ${color.text.secondary};
`

const Container = styled.div`
  --leo-control-radius: ${radius.m};

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
`

export default function SearchBox() {
  const { filteredSearchEngines, searchEngine, setSearchEngine, query, setQuery, setOpen } = useSearchContext()
  const placeholderText = searchEngine?.host === braveSearchHost
    ? 'Search the web privately'
    : 'Search the web'
  const searchInput = React.useRef<HTMLElement>()
  return <Container>
    <SearchInput tabIndex={0} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.value)} placeholder={placeholderText}>
      <Flex slot="left-icon">
        <EnginePicker positionStrategy='fixed' value={searchEngine?.keyword} onChange={e => {
          setSearchEngine(e.value!)
        }}>
          <EngineValueSlot slot="value">
            <MediumIcon src={searchEngine?.faviconUrl.url} />
          </EngineValueSlot>
          {filteredSearchEngines.map(s => <leo-option value={s.keyword} key={s.keyword}>
            <Option>
              <MediumIcon src={s.faviconUrl.url} />{s.name}
            </Option>
          </leo-option>)}
          <CustomizeButton kind="plain-faint" size="small" onClick={() => {
          history.pushState(undefined, '', '?openSettings=Search')

          // For now, close the search box - the Settings dialog doesn't use a
          // dialog, so it gets rendered underneath.
          setOpen(false)
        }}>
          Customize list
        </CustomizeButton>
      </EnginePicker>
      </Flex>
      <SearchIconContainer slot="right-icon">
        <Icon name="search" />
      </SearchIconContainer>
    </SearchInput>
  </Container>
}
