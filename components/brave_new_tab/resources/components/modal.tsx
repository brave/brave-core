/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { style } from './modal.style'

interface Props {
  isOpen: boolean
  showClose?: boolean
  onClose: () => void
  children: React.ReactNode
}

export function Modal(props: Props) {
  const dialogRef = React.useRef<HTMLDialogElement>(null)

  React.useEffect(() => {
    if (props.isOpen) {
      dialogRef.current?.showModal()
    } else {
      dialogRef.current?.close()
    }
  }, [props.isOpen])

  function maybeCloseOnBackdropEvent(event: React.MouseEvent) {
    if (!props.isOpen || !dialogRef.current) {
      return
    }
    let rect = dialogRef.current.getBoundingClientRect()
    let { clientX, clientY } = event
    if (clientX < rect.left || clientY < rect.top ||
        clientX > rect.right || clientY > rect.bottom) {
      props.onClose()
    }
  }

  return (
    <dialog
      ref={dialogRef}
      onClose={props.onClose}
      onMouseDown={maybeCloseOnBackdropEvent}
      {...style}
    >
      {props.children}
      {
        props.showClose &&
          <Button
            kind='plain-faint'
            size='medium' fab
            className='close'
            onClick={props.onClose}
          >
            <Icon name='close' />
          </Button>
      }
    </dialog>
  )
}
