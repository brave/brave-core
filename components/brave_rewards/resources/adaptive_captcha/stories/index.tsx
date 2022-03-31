/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { AdaptiveCaptchaView } from '../components/adaptive_captcha_view'

import { localeStrings } from './locale_strings'

const locale = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
}

export default {
  title: 'Rewards'
}

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

export function AdaptiveCaptcha () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div>
          <AdaptiveCaptchaView
            captchaURL=''
            captchaStatus='pending'
            onClose={actionLogger('onClose')}
            onContactSupport={actionLogger('onContactSupport')}
            onCaptchaResult={actionLogger('onCaptchaResult')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
