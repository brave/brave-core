/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { modalStyle, headerStyle, actionsStyle } from './modal.style'

function createResizeObserver() {
  // A ResizeObserver for the modal content container. When the content size
  // changes, a CSS variable "--modal-content-block-size" is set on the document
  // body. This is primarily used by browser "bubbles" in order to automatically
  // adjust the height of the bubble when a modal is displayed.
  let observer = new ResizeObserver((entries) => {
    for (const entry of entries) {
      if (entry.borderBoxSize.length > 0) {
        document.body.style.setProperty(
          '--modal-content-block-size',
          `${Math.ceil(entry.borderBoxSize[0].blockSize)}px`)
      }
    }
  })
  return {
    observe: (elem: HTMLElement) => {
      observer.observe(elem)
    },
    disconnect: () => {
      document.body.style.removeProperty('--modal-content-block-size')
      observer.disconnect()
    }
  }
}

// When switching between modals, we don't necessarily want the fade-in or
// slide-in animations to appear. When a modal is open, we add a class to the
// document body. When a modal is closed, that class will remain on the body for
// a short amount of time.
const skipAnimationsHelper = (() => {
  const className = 'modal-skip-animations'
  const delay = 250
  let timeout = 0

  return {
    onDialogRemoved() {
      if (timeout) {
        clearTimeout(timeout)
      }
      document.body.classList.add(className)
      timeout = setTimeout(() => {
        document.body.classList.remove(className)
        timeout = 0
      }, delay) as any
    }
  }
})()

interface ModalProps {
  className?: string
  onEscape?: () => void
  children: React.ReactNode
}

export function Modal(props: ModalProps) {
  const resizeObserver = React.useMemo(createResizeObserver, [])

  const onDialogElement = React.useCallback(
    (elem: HTMLDialogElement | null) => {
      if (elem) {
        elem.showModal()
        resizeObserver.observe(elem)
      } else {
        resizeObserver.disconnect()
        skipAnimationsHelper.onDialogRemoved()
      }
    }, [])

  function onKeyDown(event: React.KeyboardEvent) {
    if (event.key === 'Escape') {
      event.preventDefault()
      if (props.onEscape) {
        props.onEscape()
      }
    }
  }

  return (
    <dialog
      ref={onDialogElement}
      className={props.className}
      onKeyDown={onKeyDown}
      {...modalStyle}
    >
      {props.children}
    </dialog>
  )
}

interface ModalHeaderProps {
  title?: string
  onClose?: () => void
  closeDisabled?: boolean
}

function ModalHeader(props: ModalHeaderProps) {
  return (
    <div {...headerStyle}>
      <div className='title'>{props.title || <>&nbsp;</>}</div>
      <div className='close'>
        {
          props.onClose &&
            <button disabled={props.closeDisabled} onClick={props.onClose}>
              <Icon name='close' />
            </button>
        }
      </div>
    </div>
  )
}

interface ModalAction {
  text: string
  onClick: () => void
  className?: string
  isDisabled?: boolean
  isPrimary?: boolean
}

interface ModalActionsProps {
  actions: ModalAction[]
}

function ModalActions(props: ModalActionsProps) {
  return (
    <div {...actionsStyle}>
      {
        props.actions.map((action) => {
          let classNames: string[] = []
          if (action.className) {
            classNames.push(action.className)
          }
          if (action.isPrimary) {
            classNames.push('primary-action')
          }
          return (
            <Button
              key={action.text}
              onClick={action.onClick}
              isDisabled={action.isDisabled}
              kind={action.isPrimary ? 'filled' : 'outline'}
              className={classNames.join(' ')}
            >
              {action.text}
            </Button>
          )
        })
      }
    </div>
  )
}

Modal.Header = ModalHeader
Modal.Actions = ModalActions
