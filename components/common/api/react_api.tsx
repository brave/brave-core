// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

/**
 * Props for the generated Provider component.
 * Includes the API props plus an optional `overrides` prop for testing/storybook.
 */
type ProviderProps<API, T> = React.PropsWithChildren<
  API & {
    /**
     * Optional overrides to merge with the hook's result.
     * Useful for Storybook/tests to override specific values
     * (like `isFeedbackFormVisible`) while using real provider logic.
     */
    overrides?: Partial<T>
  }
>

// Provide a React Context provider and useAPI hook which the consumer
// can use to get the API instance and the implementer
// can customize with the required arguments for the provider.
// Optionally specify a hook to transform the API instance before returning it.
export default function generateReactContextForAPI<API extends {}, T = API>(
  hook: (api: API) => T = (api: API) => api as unknown as T,
) {
  const Context = React.createContext<T | null>(null)

  function useAPI() {
    const maybeAPI = React.useContext(Context)
    if (!maybeAPI) {
      throw new Error('useAPI must be used within a Provider')
    }
    return maybeAPI
  }

  function Provider(props: ProviderProps<API, T>) {
    const { overrides, children, ...apiProps } = props
    const value: T = hook(apiProps as API)

    // Merge overrides if provided (shallow merge)
    const finalValue = overrides ? { ...value, ...overrides } : value

    return <Context.Provider value={finalValue}>{children}</Context.Provider>
  }

  return {
    useAPI,
    Provider,
  }
}
