/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { ActionButton, CancelButton } from './action_button'
import { Modal } from '../../shared/components/modal'

import * as style from './confirmation_box.style'

interface Props {
  header: string
  text: React.ReactNode
  buttonText: string
  onConfirm: () => void
  onCancel: () => void
}

export function ConfirmationBox (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  return (
    <Modal>
      <style.root>
        <style.header>{props.header}</style.header>
        <style.text>{props.text}</style.text>
        <style.actions>
          <ActionButton onClick={props.onConfirm}>
            {props.buttonText}
          </ActionButton>
          <CancelButton onClick={props.onCancel}>
            {getString('cancel')}
          </CancelButton>
        </style.actions>
      </style.root>
    </Modal>
  )
}
