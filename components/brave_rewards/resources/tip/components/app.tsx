/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'

import { AppError } from './app_error'
import { PublisherBanner } from './publisher_banner'
import { LimitedTipForm } from './limited_tip_form'
import { TipForm } from './tip_form'
import { CloseIcon } from '../../shared/components/icons/close_icon'

import * as style from './app.style'

export function App () {
  const host = React.useContext(HostContext)

  const [hostError, setHostError] = React.useState(host.state.hostError)
  const [userType, setUserType] = React.useState(host.state.userType)

  React.useEffect(() => {
    return host.addListener((state) => {
      setHostError(state.hostError)
      setUserType(state.userType)
    })
  })

  function shouldShowFullView () {
    return userType !== 'unconnected'
  }

  function renderTipForm () {
    return shouldShowFullView() ? <TipForm /> : <LimitedTipForm />
  }

  return (
    <WithThemeVariables>
      <style.root>
        <style.banner>
          <PublisherBanner />
        </style.banner>
        <style.form>
          <style.close>
            <button onClick={host.closeDialog}><CloseIcon /></button>
          </style.close>
          {hostError ? <AppError hostError={hostError} /> : renderTipForm()}
        </style.form>
      </style.root>
    </WithThemeVariables>
  )
}
