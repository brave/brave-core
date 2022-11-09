/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'

import * as style from './app_error.style'

function getStack (error: Error) {
  const stack = error.stack || String(error)
  return stack.replace(/chrome:/g, '')
}

interface Props {
  error: Error
}

export function AppError (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  return (
    <style.root>
      <style.title>{getString('appErrorTitle')}</style.title>
      <style.details>{getStack(props.error)}</style.details>
    </style.root>
  )
}
