/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'

interface Props {
  stringKey?: string
}

function mapOnlyAnonWalletKey (key: string) {
  switch (key) {
    case 'bat': return 'bap'
    case 'batFunds': return 'bapFunds'
    case 'tokens': return 'points'
    default: return key
  }
}

export function BatString (props: Props) {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const [onlyAnonWallet, setOnlyAnonWallet] = React.useState(
    Boolean(host.state.hostError))

  React.useEffect(() => {
    return host.addListener((state) => {
      setOnlyAnonWallet(Boolean(state.onlyAnonWallet))
    })
  }, [host])

  let key = props.stringKey || 'bat'
  if (onlyAnonWallet) {
    key = mapOnlyAnonWalletKey(key)
  }

  return <>{getString(key)}</>
}
