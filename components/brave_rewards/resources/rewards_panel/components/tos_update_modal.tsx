/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext } from '../lib/host_context'
import { Modal } from '../../shared/components/modal'
import { TosUpdateNotice } from '../../shared/components/tos_update_notice'

import * as style from './tos_update_modal.style'

export function TosUpdateModal () {
  const host = React.useContext(HostContext)
  return (
    <Modal align='bottom'>
      <style.root>
        <TosUpdateNotice
          onAccept={host.acceptTermsOfServiceUpdate}
          onResetRewards={host.resetRewards}
        />
      </style.root>
    </Modal>
  )
}
