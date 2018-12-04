import * as React from 'react'
import { render } from 'react-dom'
import defaultTheme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'
import { StyleSheetManager } from 'styled-components'
import { initLocale } from 'brave-ui/helpers'
import { getUIMessages } from '../background/api/locale_api'
import TipDialogContent, { AnchorDirection } from './userTipDialogContent'

console.log('init user tip dialog')
initLocale(getUIMessages())

export default function renderTipDialog (container: Element, anchorDirection?: AnchorDirection) {
  // Keep everything contained to a closed shadow-dom
  // so that host site cannot read or write our content.
  const shadowElement = container.attachShadow({ mode: 'closed' })
  const appContainer = document.createElement('div')
  const stylesContainer = document.createElement('div')
  shadowElement.appendChild(stylesContainer)
  shadowElement.appendChild(appContainer)
  render(
    <StyleSheetManager target={stylesContainer}>
      <ThemeProvider theme={defaultTheme}>
        <TipDialogContent anchorDirection={anchorDirection} />
      </ThemeProvider>
    </StyleSheetManager>,
    appContainer
  )
}
