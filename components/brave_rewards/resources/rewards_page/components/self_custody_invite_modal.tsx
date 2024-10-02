/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Modal } from './modal'
import { useLocaleContext } from '../lib/locale_strings'

import { style } from './self_custody_invite_modal.style'

interface Props {
  onDismiss: () => void
  onConnect: () => void
}

export function SelfCustodyInviteModal(props: Props) {
  const { getString } = useLocaleContext()

  return (
    <Modal onEscape={props.onDismiss}>
      <div {...style}>
        <Modal.Header
          title={getString('selfCustodyInviteTitle')}
          onClose={props.onDismiss}
        />
        <div>{getString('selfCustodyInviteText')}</div>
        <Modal.Actions
          actions={[
            {
              text: getString('selfCustodyInviteDismissButtonLabel'),
              onClick: props.onDismiss
            },
            {
              text: getString('connectButtonLabel'),
              onClick: props.onConnect,
              isPrimary: true
            }
          ]}
        />
      </div>
    </Modal>
  )
}
