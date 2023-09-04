/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { Host } from '../lib/interfaces'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { Panel } from './panel'
import { LoadingIcon } from '../../shared/components/icons/loading_icon'

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
  const [openTime, setOpenTime] = React.useState(props.host.state.openTime)

  useHostListener(props.host, (state) => {
    setLoading(state.loading)
    setOpenTime(state.openTime)
  })

  React.useEffect(() => { props.host.onAppRendered() }, [props.host, openTime])

  // This component key is used to reset the internal view state of the
  // component tree when a cached panel is reopened.
  const panelKey = `panel-${openTime}`

  return (
    <HostContext.Provider value={props.host}>
      <WithThemeVariables>
        <style.root>
          {loading ? <Loading /> :
            <div className='rewards-panel' data-test-id='rewards-panel'>
              <Panel key={panelKey} />
            </div>
          }
        </style.root>
      </WithThemeVariables>
    </HostContext.Provider>
  )
}
