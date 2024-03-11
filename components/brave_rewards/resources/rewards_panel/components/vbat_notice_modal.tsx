/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { Modal } from '../../shared/components/modal'
import { VBATNotice } from '../../shared/components/vbat_notice'

import * as style from './vbat_notice_modal.style'

interface Props {
  onClose: () => void
  onConnectAccount: () => void
}

export function VBATNoticeModal (props: Props) {
  const host = React.useContext(HostContext)

  const [vbatDeadline, setVBATDeadline] =
    React.useState(host.state.options.vbatDeadline)

  useHostListener(host, (state) => {
    setVBATDeadline(state.options.vbatDeadline)
  })

  return (
    <Modal>
      <style.root>
        <VBATNotice
          vbatDeadline={vbatDeadline}
          onClose={props.onClose}
          onConnectAccount={props.onConnectAccount}
        />
      </style.root>
    </Modal>
  )
}
