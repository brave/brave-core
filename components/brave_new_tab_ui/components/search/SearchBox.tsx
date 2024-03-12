// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import usePromise from '$web-common/usePromise';
import Dropdown from '@brave/leo/react/dropdown';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { spacing } from '@brave/leo/tokens/css';
import { stringToMojoString16 } from 'chrome://resources/js/mojo_type_util.js';
import { AutocompleteResult, OmniboxPopupSelection, PageHandler, PageHandlerRemote, PageInterface, PageReceiver } from 'gen/components/omnibox/browser/omnibox.mojom.m';
import * as React from 'react';
import styled from 'styled-components';
import getNTPBrowserAPI from '../../api/background';
import { getDefaultSearchEngine, isSearchEngineEnabled, setDefaultSearchEngine } from './config';
import { LargeIcon, MediumIcon } from './SearchEngineIcon';

const SearchInput = styled(Input)`
  --leo-control-padding: 6px;

  display: inline-block;
  width: 540px;
`

const EnginePicker = styled(Dropdown)`
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Option = styled.div`
    display: flex;
    gap: ${spacing.m};
`

export const searchEnginesPromise = getNTPBrowserAPI().pageHandler.getSearchEngines().then(r => r.searchEngines)

export const omniboxController: PageHandlerRemote = PageHandler.getRemote();
(window as any).omnibox = omniboxController;

class SearchPage implements PageInterface {
  private receiver = new PageReceiver(this)
  private result: AutocompleteResult | undefined
  private resultListeners: Array<(result?: AutocompleteResult) => void> = []
  private selectionListeners: Array<(selection: OmniboxPopupSelection) => void> = []

  constructor() {
    omniboxController.setPage(this.receiver.$.bindNewPipeAndPassRemote())
  }

  addResultListener(listener: (result?: AutocompleteResult) => void) {
    this.resultListeners.push(listener)
    if (this.result) listener(this.result)
  }

  removeResultListener(listener: (result?: AutocompleteResult) => void) {
    this.resultListeners = this.resultListeners.filter(r => r !== listener)
  }

  addSelectionListener(listener: (selection: OmniboxPopupSelection) => void) {
    this.selectionListeners.push(listener)
  }

  removeSelectionListener(listener: (selection: OmniboxPopupSelection) => void) {
    this.selectionListeners = this.selectionListeners.filter(s => s !== listener)
  }

  autocompleteResultChanged(result: AutocompleteResult) {
    this.result = result;
    for (const listener of this.resultListeners) listener(result)
  }

  updateSelection(selection: OmniboxPopupSelection) {
    for (const listener of this.selectionListeners) listener(selection)
  }
}

export const search = new SearchPage()

export default function SearchBox() {
  const [query, setQuery] = React.useState('')
  const { result: searchEngines = [] } = usePromise(() => searchEnginesPromise, [])
  const filteredSearchEngines = React.useMemo(() => searchEngines.filter(isSearchEngineEnabled), [searchEngines])
  const [keyword, setKeyword] = React.useState<string>()

  // When we receive search engines, use the first one as our keyword, if we don't have a match.
  React.useEffect(() => {
    if (!searchEngines.length) return

    const match = filteredSearchEngines.find(s => s.origin === getDefaultSearchEngine())
      ?? searchEngines[0]
    setKeyword(match.keyword)
  }, [filteredSearchEngines])

  React.useEffect(() => {
    if (query) {
      const keywordQuery = `${keyword} ${query}`
      omniboxController.queryAutocomplete(stringToMojoString16(keywordQuery), false);
    } else {
      omniboxController.stopAutocomplete(true)
    }
  }, [query, keyword])

  const searchInput = React.useRef<HTMLElement>()
  return <SearchInput tabIndex={0} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.detail.value)} placeholder="Search the web privately">
    <Flex slot="left-icon">
      <EnginePicker positionStrategy='fixed' value={keyword} onChange={e => {
        setKeyword(e.detail.value)
        setDefaultSearchEngine(searchEngines.find(s => s.keyword === e.detail.value)!)
      }}>
        <div slot="value">
          <LargeIcon src={searchEngines.find(s => s.keyword === keyword)?.faviconUrl.url} />
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
