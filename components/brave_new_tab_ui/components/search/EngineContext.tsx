// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { SearchEngineInfo } from '../../api/background'
import { useNewTabPref } from '../../hooks/usePref'
import * as React from 'react';
import { ENABLED_SEARCH_ENGINES_KEY, LAST_SEARCH_ENGINE_KEY, braveSearchHost } from './config';

interface Engine {
  setLastSearchEngine: (engine: SearchEngineInfo) => void
  lastSearchEngine: string,
  engineConfig: Record<string, boolean>,
  setEngineConfig: (engine: string, enabled: boolean) => void,
}

const EngineContext = React.createContext<Engine>({
  setLastSearchEngine: () => { },
  lastSearchEngine: braveSearchHost,
  engineConfig: {},
  setEngineConfig: () => { },
})

const searchEngineConfig = () => {
  const localStorageValue = localStorage.getItem(ENABLED_SEARCH_ENGINES_KEY);
  if (localStorageValue) {
    return JSON.parse(localStorageValue);
  }
  return {
    // Default to enabling Brave Search
    [braveSearchHost]: true
  };
}

export function EngineContextProvider(props: React.PropsWithChildren<{}>) {
  const [lastUsed, setLastUsed] = useNewTabPref('lastUsedNtpSearchEngine')
  const lastLocalStorage = localStorage.getItem(LAST_SEARCH_ENGINE_KEY)
  const [config, setConfig] = React.useState<Record<string, boolean>>(searchEngineConfig);

  const setLastNtpSearchEngine = React.useCallback((engine: SearchEngineInfo) => {
    setLastUsed(engine.host);
  }, [setLastUsed])

  const lastNtpSearchEngine = React.useMemo(() => {
    let last = lastUsed;

    // Migrates the existing local storage value to the new pref value, letting the local storage value take precedence.
    if (lastLocalStorage && config[lastLocalStorage] && lastUsed !== lastLocalStorage) {
      setLastUsed(lastLocalStorage);
      last = lastLocalStorage;
      localStorage.removeItem(LAST_SEARCH_ENGINE_KEY)
    }

    // If the last search engine we used has been disabled or doesn't exist in the config, return the first enabled
    // one, or Brave Search.
    // Note: The key for `Google` is the empty string which is falsey so we need
    // to check for undefined here.
    if (last === undefined || !config[last]) {
      return Object.keys(config).find(key => config[key]) ?? braveSearchHost
    }

    return last
  }, [lastUsed, lastLocalStorage, config]);

  const setEngineEnabled = React.useCallback(((engine: string, enabled: boolean) => {
    setConfig(prevConfig => {
      const newState = { ...prevConfig, [engine]: enabled };
      localStorage.setItem(ENABLED_SEARCH_ENGINES_KEY, JSON.stringify(newState));
      return newState;
    });
  }), [])

  return (
    <EngineContext.Provider value={{
      setLastSearchEngine: setLastNtpSearchEngine,
      lastSearchEngine: lastNtpSearchEngine,
      engineConfig: config,
      setEngineConfig: setEngineEnabled,
    }} >
      {props.children}
    </EngineContext.Provider>
  )
}

export function useEngineContext() {
  return React.useContext(EngineContext)
}
