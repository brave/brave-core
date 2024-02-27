/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions } from '../lib/redux_hooks'
import { Modal } from '../../shared/components/modal'
import { TosUpdateNotice } from '../../shared/components/tos_update_notice'

import * as style from './tos_update_modal.style'

export function TosUpdateModal () {
  const actions = useActions()

  return (
    <Modal>
      <style.root>
        <TosUpdateNotice
          onAccept={actions.acceptTermsOfServiceUpdate}
          onResetRewards={actions.onModalResetOpen}
        />
      </style.root>
    </Modal>
  )
}
