/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { CloseIcon } from './icons/close_icon'

const style = {
  root: styled.div`
    position: fixed;
    top: 0;
    left: 0;
    bottom: 0;
    right: 0;
    overflow: auto;
    background: rgba(0, 0, 0, 0.7);
    z-index: 9999;
    display: flex;
    flex-direction: column;
    align-items: center;
  `,

  topSpacer: styled.div`
    flex: 45 0 auto;
  `,

  content: styled.div`
    flex: 0 0 auto;
  `,

  bottomSpacer: styled.div`
    flex: 55 0 auto;
  `,

  close: styled.div`
    color: var(--brave-palette-neutral600);
    text-align: right;

    button {
      margin: 0;
      padding: 2px;
      background: none;
      border: none;
      cursor: pointer;
    }

    .icon {
      display: block;
      width: 14px;
      height: auto;
    }
  `
}

interface ModalProps {
  children: React.ReactNode
}

export function Modal (props: ModalProps) {
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

  const onMountUnmount = (elem: HTMLElement | null) => {
    if (elem) {
      resizeObserver.observe(elem)
    } else {
      resizeObserver.disconnect()
      document.body.style.removeProperty('--modal-content-block-size')
    }
  }

  return (
    <style.root>
      <style.topSpacer />
      <style.content ref={onMountUnmount}>
        {props.children}
      </style.content>
      <style.bottomSpacer />
    </style.root>
  )
}

interface ModalCloseButtonProps {
  onClick: () => void
}

export function ModalCloseButton (props: ModalCloseButtonProps) {
  return (
    <style.close>
      <button onClick={props.onClick}><CloseIcon /></button>
    </style.close>
  )
}
