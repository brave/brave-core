// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import Dropdown from '@brave/leo/react/dropdown';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { radius, spacing } from '@brave/leo/tokens/css';
import * as React from 'react';
import styled from 'styled-components';
import { LargeIcon, MediumIcon } from './SearchEngineIcon';
import { useSearchContext } from './SearchContext';

const SearchInput = styled(Input)`
  --leo-control-padding: 6px;

  display: inline-block;
  width: 540px;

  /* If we have search results, don't add a radius to the bottom of the search box */
  &:has(+ div) {
    --leo-control-radius: ${radius.m} ${radius.m} 0 0;
  }
`

const EnginePicker = styled(Dropdown)`
  --leo-control-radius: ${spacing.m};
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Option = styled.div`
    display: flex;
    gap: ${spacing.m};
`

export default function SearchBox() {
  const { filteredSearchEngines, searchEngine, setSearchEngine, query, setQuery } = useSearchContext()
  const searchInput = React.useRef<HTMLElement>()
  return <SearchInput tabIndex={0} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.detail.value)} placeholder="Search the web privately">
    <Flex slot="left-icon">
      <EnginePicker positionStrategy='fixed' value={searchEngine?.keyword} onChange={e => {
        setSearchEngine(e.detail.value)
      }}>
        <div slot="value">
          <LargeIcon src={searchEngine?.faviconUrl.url} />
        </div>
        {filteredSearchEngines.map(s => <leo-option value={s.keyword} key={s.keyword}>
          <Option>
            <MediumIcon src={s.faviconUrl.url} />{s.name}
          </Option>
        </leo-option>)}
      </EnginePicker>
    </Flex>
    <SearchIconContainer slot="right-icon">
      <Icon name="search" />
    </SearchIconContainer>
  </SearchInput>
}
