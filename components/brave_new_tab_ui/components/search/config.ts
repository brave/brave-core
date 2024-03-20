// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { SearchEngineInfo } from "../../api/background";

// At some point we might want to store this in prefs, but
// at this point its only used on Desktop, and only on the NTP
// so this seems fine.
const ENABLED_SEARCH_ENGINES_KEY = 'search-engines'
const LAST_SEARCH_ENGINE_KEY = 'last-search-engine'

export const braveSearchHost = 'search.brave.com'

let cache: Record<string, boolean> | undefined

const getConfig = () => {
  if (!cache) {
    cache = JSON.parse(localStorage.getItem(ENABLED_SEARCH_ENGINES_KEY)!) ?? {
      // Default to enabling Brave Search
      [braveSearchHost]: true
    }
  }
  return cache!
}

export const setEngineEnabled = (engine: SearchEngineInfo, enabled: boolean) => {
  const config = getConfig()
  config[engine.host] = enabled

  localStorage.setItem(ENABLED_SEARCH_ENGINES_KEY, JSON.stringify(config))
}

export const isSearchEngineEnabled = (engine: SearchEngineInfo) => getConfig()[engine.host]

export const getDefaultSearchEngine = () => {
  return localStorage.getItem(LAST_SEARCH_ENGINE_KEY) ?? braveSearchHost
}

export const setDefaultSearchEngine = (engine: SearchEngineInfo) => {
  localStorage.setItem(LAST_SEARCH_ENGINE_KEY, engine.host)
}
