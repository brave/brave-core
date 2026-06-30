/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { NtpWidget } from './ntp_widget'
import { WidgetMenu } from './widget_menu'
import {
  CustomWidget as CustomWidgetData,
  removeCustomWidget,
} from '../../state/custom_widgets_store'

import { style } from './custom_widget.style'

// Origin of the isolated, sandboxed host that renders untrusted widget markup.
// Must match kBraveNTPWidgetsUIURL in custom_widget_untrusted_ui.cc.
const widgetOrigin = 'chrome-untrusted://brave-ntp-widgets'
const widgetUrl = `${widgetOrigin}/host.html`

interface Props {
  widget: CustomWidgetData
}

export function CustomWidget(props: Props) {
  const frameRef = React.useRef<HTMLIFrameElement>(null)
  const { id, name, html } = props.widget

  React.useEffect(() => {
    function onMessage(event: MessageEvent) {
      if (event.origin !== widgetOrigin) {
        return
      }
      const frame = frameRef.current
      if (!frame || event.source !== frame.contentWindow) {
        return
      }
      if (event.data && event.data.type === 'brave-ntp-widget-ready') {
        frame.contentWindow?.postMessage(
          { type: 'brave-ntp-widget', html },
          widgetOrigin,
        )
      }
    }
    window.addEventListener('message', onMessage)
    return () => window.removeEventListener('message', onMessage)
  }, [html])

  return (
    <NtpWidget>
      <div data-css-scope={style.scope}>
        <WidgetMenu>
          <leo-menu-item onClick={() => removeCustomWidget(id)}>
            <Icon name='trash' /> Remove widget
          </leo-menu-item>
        </WidgetMenu>
        <iframe
          ref={frameRef}
          className='widget-frame'
          src={widgetUrl}
          title={name}
        />
      </div>
    </NtpWidget>
  )
}
