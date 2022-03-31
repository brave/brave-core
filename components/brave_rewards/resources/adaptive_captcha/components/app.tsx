/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { Host } from '../lib/interfaces'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { AdaptiveCaptchaView } from './adaptive_captcha_view'

interface Props {
  host: Host
}

export function App (props: Props) {
  const { host } = props

  const [captchaURL, setCaptchaURL] = React.useState(host.state.captchaURL)
  const [captchaStatus, setCaptchaStatus] =
    React.useState(host.state.captchaStatus)

  useHostListener(props.host, (state) => {
    setCaptchaURL(state.captchaURL)
    setCaptchaStatus(state.captchaStatus)
  })

  function onClose () {
    window.close()
  }

  function onContactSupport () {
    window.open('https://support.brave.com/', '_blank')
  }

  return (
    <HostContext.Provider value={props.host}>
      <LocaleContext.Provider value={props.host}>
        <WithThemeVariables>
          <AdaptiveCaptchaView
            captchaURL={captchaURL}
            captchaStatus={captchaStatus}
            onClose={onClose}
            onContactSupport={onContactSupport}
            onCaptchaResult={host.handleCaptchaResult}
          />
        </WithThemeVariables>
      </LocaleContext.Provider>
    </HostContext.Provider>
  )
}
