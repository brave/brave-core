/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import {
  ModalStyle,
  CloseButtonStyle,
  HeaderStyle,
  ActionsStyle } from './modal.style'

interface ModalProps {
  children: React.ReactNode
}

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

export function Modal(props: ModalProps) {
  // Attach a ResizeObserver for the modal content container. When the content
  // size changes, set a CSS variable "--modal-content-size" on the document
  // body. This is primarily used by browser "bubbles" in order to automatically
  // adjust the height of the bubble when a modal is displayed.
  const resizeObserver = React.useMemo(() => {
    return new ResizeObserver((entries) => {
      for (const entry of entries) {
        if (entry.borderBoxSize.length > 0) {
          document.body.style.setProperty(
            '--modal-content-block-size',
            `${entry.borderBoxSize[0].blockSize}px`)
        }
      }
    })
  }, [])

  function onMountUnmount(elem: HTMLElement | null) {
    if (elem) {
      resizeObserver.observe(elem)
    } else {
      resizeObserver.disconnect()
      document.body.style.removeProperty('--modal-content-block-size')
      startAnimationCoolDown()
    }
  }

  return (
    <ModalStyle>
      <div className='modal-backdrop'>
        <div className='top-spacer' />
        <div className='modal-content' ref={onMountUnmount}>
          {props.children}
        </div>
        <div className='bottom-spacer' />
      </div>
    </ModalStyle>
  )
}

interface ModalCloseButtonProps {
  isDisabled?: boolean
  onClick: () => void
}

export function ModalCloseButton(props: ModalCloseButtonProps) {
  return (
    <CloseButtonStyle>
      <button
        disabled={props.isDisabled}
        onClick={props.onClick}
      >
        <Icon name='close' />
      </button>
    </CloseButtonStyle>
  )
}

interface ModalHeaderProps {
  title?: string
  onCloseClick?: () => void
  isCloseDisabled?: boolean
}

export function ModalHeader(props: ModalHeaderProps) {
  return (
    <HeaderStyle>
      <div className='title'>{props.title || <>&nbsp;</>}</div>
      <div className='close'>
        {
          props.onCloseClick &&
            <ModalCloseButton
              onClick={props.onCloseClick}
              isDisabled={props.isCloseDisabled}
            />
        }
      </div>
    </HeaderStyle>
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

export function ModalActions(props: ModalActionsProps) {
  return (
    <ActionsStyle>
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
    </ActionsStyle>
  )
}
