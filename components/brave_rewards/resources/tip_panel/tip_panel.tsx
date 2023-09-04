/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import { setIconBasePath } from '@brave/leo/react/icon'

import { LocaleContext } from '../shared/lib/locale_context'
import { TabOpenerContext } from '../shared/components/new_tab_link'
import { ModelContext } from './lib/model_context'
import { createLocaleContextForWebUI } from '../shared/lib/webui_locale_context'
import { createModel } from './lib/webui_model'
import { TipPanelProxy } from './lib/tip_panel_proxy'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

// A wrapping component that sets the document dimensions. This is necessary
// when rendering a WebUI bubble, as the bubble will resize based on the
// explicit rendered dimensions of the web content, and we want to adjust the
// layout depending on the size of the browser window to which the bubble is
// attached.
function Dimensions (props: { children: React.ReactNode }) {
  type Size = { width: number, height: number }

  const proxy = TipPanelProxy.getInstance()
  const [browserSize, setBrowserSize] = React.useState<Size | null>(null)

  React.useEffect(() => {
    proxy.handler.getBrowserSize().then((size) => {
      setBrowserSize(size)
    })
  }, [])

  React.useEffect(() => {
    if (browserSize) {
      proxy.handler.showUI()
    }
  }, [browserSize])

  if (!browserSize) {
    return null
  }

  const maxWidth = browserSize.width - 20
  const isNarrow = maxWidth <= 999

  function getStyleProps (): React.CSSProperties {
    if (!browserSize) {
      return {}
    }

    if (!isNarrow) {
      return {
        width: Math.min(1150, maxWidth) + 'px',
      }
    }

    return {
      width: '550px',
      maxHeight: Math.max(browserSize.height, 600) + 'px',
      overflowY: 'auto'
    }
  }

  return (
    <div className={isNarrow ? 'narrow-view': ''} style={getStyleProps()}>
      {props.children}
    </div>
  )
}

function onReady () {
  const tabOpener = {
    openTab (url: string) { TipPanelProxy.getInstance().handler.openTab(url) }
  }

  ReactDOM.render(
    <ModelContext.Provider value={createModel()}>
      <TabOpenerContext.Provider value={tabOpener}>
        <LocaleContext.Provider value={createLocaleContextForWebUI()}>
          <Dimensions>
            <App />
          </Dimensions>
        </LocaleContext.Provider>
      </TabOpenerContext.Provider>
    </ModelContext.Provider>,
    document.getElementById('root'))
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', onReady)
} else {
  onReady()
}
