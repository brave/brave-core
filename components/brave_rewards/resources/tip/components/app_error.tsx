/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostError } from '../lib/interfaces'
import { LocaleContext } from '../../shared/lib/locale_context'

import * as style from './app_error.style'

interface Props {
  hostError: HostError
}

function getErrorKey (type: string) {
  switch (type) {
    case 'ERR_FETCH_BALANCE':
      return 'errorServerConnection'
    case 'ERR_TIP_FAILED':
      return 'errorTipFailed'
    default:
      return 'errorHasOccurred'
  }
}

function getClassName (type: string) {
  switch (type) {
    case 'ERR_FETCH_BALANCE':
      return 'server-error'
    case 'ERR_TIP_FAILED':
      return 'tipping-error'
    default:
      return ''
  }
}

export function AppError (props: Props) {
  const { hostError } = props
  const locale = React.useContext(LocaleContext)

  return (
    <style.root>
      <style.graphic className={getClassName(hostError.type)} />
      <style.heading>
        {locale.getString(getErrorKey(hostError.type))}
      </style.heading>
      <style.message>
        {locale.getString('errorTryAgain')}
      </style.message>
      <style.details>
        {hostError.type}{hostError.code ? ` : ${hostError.code}` : ''}
      </style.details>
    </style.root>
  )
}
