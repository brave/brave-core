/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { HostContext } from '../lib/host_context'
import { Host } from '../lib/interfaces'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { Panel } from './panel'

interface Props {
  host: Host
}

export function App (props: Props) {
  return (
    <HostContext.Provider value={props.host}>
      <LocaleContext.Provider value={props.host}>
        <WithThemeVariables>
          <Panel />
        </WithThemeVariables>
      </LocaleContext.Provider>
    </HostContext.Provider>
  )
}
