// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Flex from '$web-common/Flex';
import Dropdown from '@brave/leo/react/dropdown';
import Icon from '@brave/leo/react/icon';
import Input from '@brave/leo/react/input';
import { icon, spacing } from '@brave/leo/tokens/css';
import { stringToMojoString16 } from 'chrome://resources/js/mojo_type_util.js';
import { AutocompleteResult, OmniboxPopupSelection, PageHandler, PageHandlerRemote, PageInterface, PageReceiver } from 'gen/components/omnibox/browser/omnibox.mojom.m';
import * as React from 'react';
import styled from 'styled-components';

interface SearchEngine {
  name: string,
  icon: string
}
const searchEngines: SearchEngine[] = [{
  name: 'Brave',
  icon: 'brave-icon-search-color'
}, {
  name: 'Google',
  icon: 'google-color'
}, {
  name: 'DuckDuckGo',
  icon: 'duckduckgo-color'
}, {
  name: 'Bing',
  icon: 'bing-color'
}]

const SearchInput = styled(Input)`
  --leo-control-padding: 6px;

  display: inline-block;
  width: 540px;
`

const EnginePicker = styled(Dropdown)`
`

const SelectedIcon = styled(Icon)`
  --leo-icon-size: ${icon.l};
`

const SearchIconContainer = styled.div`
  padding-right: ${spacing.m};
`

const Option = styled.div`
    display: flex;
    gap: ${spacing.m};
`

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
  const [searchEngine, setSearchEngine] = React.useState('Brave')

  React.useEffect(() => {
    omniboxController.queryAutocomplete(stringToMojoString16(query), false);
  }, [query])

  const searchInput = React.useRef<HTMLElement>()
  return <SearchInput tabIndex={0} type="text" ref={searchInput} value={query} onInput={e => setQuery(e.detail.value)} placeholder="Search the web privately">
    <Flex slot="left-icon">
      <EnginePicker positionStrategy='fixed' value={searchEngine} onChange={e => setSearchEngine(e.detail.value)}>
        <div slot="value">
          <SelectedIcon name={searchEngines.find(s => s.name === searchEngine)?.icon} />
        </div>
        {searchEngines.map(s => <leo-option value={s.name} key={s.name}>
          <Option>
            <Icon name={s.icon} />{s.name}
          </Option>
        </leo-option>)}
      </EnginePicker>
    </Flex>
    <SearchIconContainer slot="right-icon">
      <Icon name="search" />
    </SearchIconContainer>
  </SearchInput>
}
