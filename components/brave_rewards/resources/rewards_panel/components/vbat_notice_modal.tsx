/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { Modal } from '../../shared/components/modal'
import { VBATNotice } from '../../shared/components/vbat_notice'

import * as derivedState from '../lib/derived_state'
import * as style from './vbat_notice_modal.style'

interface Props {
  onClose: () => void
  onConnectAccount: () => void
}

export function VBATNoticeModal (props: Props) {
  const host = React.useContext(HostContext)

  const [vbatDeadline, setVBATDeadline] =
    React.useState(host.state.options.vbatDeadline)
  const [canConnect, setCanConnect] =
    React.useState(derivedState.canConnectAccount(host.state))
  const [declaredCountry, setDeclaredCountry] =
    React.useState(host.state.declaredCountry)

  useHostListener(host, (state) => {
    setVBATDeadline(state.options.vbatDeadline)
    setCanConnect(derivedState.canConnectAccount(state))
    setDeclaredCountry(state.declaredCountry)
  })

  return (
    <Modal>
      <style.root>
        <VBATNotice
          vbatDeadline={vbatDeadline}
          canConnectAccount={canConnect}
          declaredCountry={declaredCountry}
          onClose={props.onClose}
          onConnectAccount={props.onConnectAccount}
        />
      </style.root>
    </Modal>
  )
}
