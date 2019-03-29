import * as React from 'react'
import { ThemeProvider } from 'brave-ui/theme'
import IBraveTheme from 'brave-ui/theme/theme-interface'

export type Props = {
  initialThemeType?: chrome.braveTheme.ThemeType
  dark: IBraveTheme,
  light: IBraveTheme
}
type State = {
  themeType?: chrome.braveTheme.ThemeType
}

function themeTypeToState (themeType: chrome.braveTheme.ThemeType): State {
  return {
    themeType
  }
}

export default class BraveCoreThemeProvider extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    if (props.initialThemeType) {
      this.state = themeTypeToState(props.initialThemeType)
    }
    chrome.braveTheme.onBraveThemeTypeChanged.addListener(this.setThemeState)
  }

  setThemeState = (themeType: chrome.braveTheme.ThemeType) => {
    this.setState(themeTypeToState(themeType))
  }

  render () {
    // Don't render until we have a theme
    if (!this.state.themeType) return null
    // Render provided dark or light theme
    const selectedShieldsTheme = this.state.themeType === 'Dark'
                ? this.props.dark
                : this.props.light
    return (
      <ThemeProvider theme={selectedShieldsTheme}>
        {React.Children.only(this.props.children)}
      </ThemeProvider>
    )
  }
}
