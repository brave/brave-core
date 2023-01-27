// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { ThemeProvider } from 'styled-components'
import LeoTheme from '@brave/leo/tokens/styledComponents/theme'
import DefaultLegacyTheme from 'brave-ui/theme/brave-default'
import DefaultDarkLegacyTheme from 'brave-ui/theme/brave-dark'
import IBraveUITheme from 'brave-ui/theme/theme-interface'

export type Props = {
  // Deprecated: `initialThemeType` is not used!
  initialThemeType?: any
  // Deprecated: Any extra theme properties should be provided by a custom
  // ThemeProvider local to a specific component. The definition would need
  // to be provided to styled-components' DefaultTheme via a separate
  // styled-components-theme.d.ts local to that component.
  legacyDarkTheme?: IBraveUITheme
  // Deprecated: Any extra theme properties should be provided by a custom
  // ThemeProvider local to a specific component. The definition would need
  // to be provided to styled-components' DefaultTheme via a separate
  // styled-components-theme.d.ts local to that component.
  legacyLightTheme?: IBraveUITheme
}

const darkModeMediaMatcher = window.matchMedia('(prefers-color-scheme: dark)')

/**
 * Provides a boolean value indicating whether the user's preferred color scheme
 * is dark or not.
 */
export const DarkColorSchemeContext = React.createContext<boolean>(darkModeMediaMatcher.matches)

export default function LightDarkThemeProvider (props: React.PropsWithChildren<Props>) {
  const [isDarkMode, setIsDarkMode] = React.useState(darkModeMediaMatcher.matches)

  React.useEffect(() => {
    const handleDarkModeChange = (e: MediaQueryListEvent) => {
      setIsDarkMode(e.matches)
    }

    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        setIsDarkMode(darkModeMediaMatcher.matches)
      }
    }

    darkModeMediaMatcher.addEventListener('change', handleDarkModeChange)
    document.addEventListener('visibilitychange', onVisibilityChange)

    return () => {
      darkModeMediaMatcher.removeEventListener('change', handleDarkModeChange)
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  const selectedLegacyTheme = isDarkMode
    ? props.legacyDarkTheme || DefaultDarkLegacyTheme
    : props.legacyLightTheme || DefaultLegacyTheme

  const selectedThemeWithLeo = React.useMemo(() => {
    // Always add the Leo theme, there is no separate dark or light version
    // of it.
    return {
      ...LeoTheme,
      legacy: selectedLegacyTheme
    }
  }, [selectedLegacyTheme])

  return (
    <ThemeProvider theme={selectedThemeWithLeo}>
      <DarkColorSchemeContext.Provider value={isDarkMode}>
        {React.Children.only(props.children)}
      </DarkColorSchemeContext.Provider>
    </ThemeProvider>
  )
}
