/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { Host } from '../lib/interfaces'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { Panel } from './panel'
import { LoadingIcon } from './icons/loading_icon'

import * as style from './app.style'

interface Props {
  host: Host
}

function Loading () {
  return (
    <style.loading>
      <LoadingIcon />
    </style.loading>
  )
}

export function App (props: Props) {
  const [loading, setLoading] = React.useState(props.host.state.loading)

  useHostListener(props.host, (state) => {
    setLoading(state.loading)
  })

  return (
    <HostContext.Provider value={props.host}>
      <LocaleContext.Provider value={props.host}>
        <WithThemeVariables>
          <style.root>
            {loading ? <Loading /> : <Panel />}
          </style.root>
        </WithThemeVariables>
      </LocaleContext.Provider>
    </HostContext.Provider>
  )
}
