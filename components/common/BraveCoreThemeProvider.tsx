import * as React from 'react'
import { ThemeProvider } from 'styled-components'
import IBraveTheme from 'brave-ui/theme/theme-interface'

export type Props = {
  initialThemeType?: chrome.braveTheme.ThemeType
  dark: IBraveTheme
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
    // Ensure we have access to braveTheme before updating.
    // Otherwise this would break Storybook.
    if (chrome.braveTheme) {
      chrome.braveTheme.onBraveThemeTypeChanged.addListener(this.setThemeState)
    }
  }

  setThemeState = (themeType: chrome.braveTheme.ThemeType) => {
    this.setState(themeTypeToState(themeType))
  }

  componentDidUpdate (prevProps: Props) {
    // Update theme based on React prop changes.
    // This only runs on storybook and is needed
    // since it has no access to chrome.* APIs
    if (chrome.braveTheme) {
      return
    }
    if (prevProps.initialThemeType !== this.props.initialThemeType) {
      this.setThemeState(this.props.initialThemeType || 'System')
    }
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
