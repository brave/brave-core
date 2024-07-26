/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { modalStyle, headerStyle, actionsStyle } from './modal.style'

// When switching between modals, we don't want the fade-in or slide-in
// animations to appear. When a modal is closed, we add a class to the document
// body for a short period of time which is used to disable animations in CSS.
let animationCoolDownTimeout = 0
const animationCoolDownDelay = 500

function startAnimationCoolDown() {
  const cssClass = 'modal-skip-animations'
  if (animationCoolDownTimeout) {
    clearTimeout(animationCoolDownTimeout)
  }
  document.body.classList.add(cssClass)
  animationCoolDownTimeout = setTimeout(() => {
    document.body.classList.remove(cssClass)
    animationCoolDownTimeout = 0
  }, animationCoolDownDelay) as any // `any` to work around `Timeout` mis-typing
}

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

interface ModalProps {
  className?: string
  onEscape?: () => void
  children: React.ReactNode
}

export function Modal(props: ModalProps) {
  const resizeObserver = React.useMemo(createResizeObserver, [])

  function onDialogElement(elem: HTMLDialogElement | null) {
    if (elem) {
      elem.showModal()
      resizeObserver.observe(elem)
    } else {
      resizeObserver.disconnect()
      startAnimationCoolDown()
    }
  }

  function onKeyDown(event: React.KeyboardEvent) {
    if (event.key === 'Escape') {
      event.preventDefault()
      if (props.onEscape) {
        props.onEscape()
      }
    }
  }

  function onAnimationStart(event: React.AnimationEvent<HTMLDialogElement>) {
    event.currentTarget.classList.add('app-modal-animating')
  }

  function onAnimationEnd(event: React.AnimationEvent<HTMLDialogElement>) {
    event.currentTarget.classList.remove('app-modal-animating')
  }

  return (
    <dialog
      ref={onDialogElement}
      className={props.className}
      onKeyDown={onKeyDown}
      onAnimationStart={onAnimationStart}
      onAnimationEnd={onAnimationEnd}
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
