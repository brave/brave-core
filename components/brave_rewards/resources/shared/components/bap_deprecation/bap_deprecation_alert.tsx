/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { Modal, ModalCloseButton } from '../modal'
import { WarningIcon } from './icons/warning_icon'

import * as style from './bap_deprecation_alert.style'

interface Props {
  onClose: () => void
}

export function BAPDeprecationAlert (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <Modal>
      <style.root>
        <ModalCloseButton onClick={props.onClose} />
        <style.banner><WarningIcon /></style.banner>
        <style.content>
          <style.header>
            {getString('bapDeprecationHeader')}
          </style.header>
          <style.text>
            {getString('bapDeprecationAlertText')}
          </style.text>
          <style.action>
            <button onClick={props.onClose}>
              {getString('bapDeprecationOK')}
            </button>
          </style.action>
        </style.content>
      </style.root>
    </Modal>
  )
}
