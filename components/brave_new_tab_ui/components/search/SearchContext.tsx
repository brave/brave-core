// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import usePromise from '$web-common/usePromise';
import { AutocompleteResult, OmniboxPopupSelection, PageHandler, PageHandlerRemote, PageInterface, PageReceiver } from 'gen/components/omnibox/browser/omnibox.mojom.m';
import { stringToMojoString16 } from 'chrome://resources/js/mojo_type_util.js';
import * as React from 'react';
import getNTPBrowserAPI, { SearchEngineInfo } from '../../api/background';
import { getDefaultSearchEngine, isSearchEngineEnabled, setDefaultSearchEngine } from './config';

interface Context {
  open: boolean,
  setOpen: (open: boolean) => void,
  query: string,
  setQuery: (query: string) => void
  searchEngine?: SearchEngineInfo,
  setSearchEngine: (searchEngine: SearchEngineInfo | string) => void
  searchEngines: SearchEngineInfo[]
  filteredSearchEngines: SearchEngineInfo[]
}

const Context = React.createContext<Context>({
  open: false,
  setOpen: () => {},
  query: '',
  setQuery: () => { },
  searchEngine: undefined,
  setSearchEngine: () => { },
  searchEngines: [],
  filteredSearchEngines: []
})

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

export function SearchContext(props: React.PropsWithChildren<{}>) {
  const [open, setOpen] = React.useState(false)
  const [searchEngine, setSearchEngineInternal] = React.useState<SearchEngineInfo>()
  const [query, setQuery] = React.useState('')
  const { result: searchEngines = [] } = usePromise(() => searchEnginesPromise, [])
  const filteredSearchEngines = React.useMemo(() => searchEngines.filter(isSearchEngineEnabled), [searchEngines])

  const setSearchEngine = React.useCallback((engine: SearchEngineInfo | string) => {
    if (typeof engine === 'string') {
      engine = searchEngines.find(e => e.host === engine || e.keyword === engine)!
    }

    if (!engine) return

    setDefaultSearchEngine(engine)
    setSearchEngineInternal(engine)
  }, [searchEngines]);

  // When we receive search engines, use the first one as our keyword, if we don't have a match.
  React.useEffect(() => {
    if (!searchEngines.length) return

    const match = filteredSearchEngines.find(s => s.host === getDefaultSearchEngine())
      ?? searchEngines[0]
    setSearchEngine(match)
  }, [filteredSearchEngines])

  // When the query changes, notify the browser side.
  React.useEffect(() => {
    if (query) {
      const keywordQuery = `${searchEngine?.keyword} ${query}`
      omniboxController.queryAutocomplete(stringToMojoString16(keywordQuery), false);
    } else {
      omniboxController.stopAutocomplete(true)
    }
  }, [query, searchEngine])

  const context = React.useMemo(() => ({
    open,
    setOpen,
    searchEngine,
    setSearchEngine,
    query,
    setQuery,
    searchEngines,
    filteredSearchEngines
  }), [searchEngine, setSearchEngine, filteredSearchEngines, query, searchEngines, open])

  return <Context.Provider value={context}>
    {props.children}
  </Context.Provider>
}

export function useSearchContext() {
  return React.useContext(Context)
}
