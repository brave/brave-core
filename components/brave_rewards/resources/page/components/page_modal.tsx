/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Modal, ModalCloseButton } from '../../shared/components/modal'

import * as style from './page_modal.style'

interface Props {
  title: string
  children: React.ReactNode
  onClose: () => void
}

export function PageModal (props: Props) {
  return (
    <Modal>
      <style.root>
        <style.header>
          <style.title>
            {props.title}
          </style.title>
          <style.close>
            <ModalCloseButton onClick={props.onClose} />
          </style.close>
        </style.header>
        {props.children}
      </style.root>
    </Modal>
  )
}
