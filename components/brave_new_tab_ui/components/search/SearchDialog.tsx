// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color } from '@brave/leo/tokens/css/variables';
import * as React from 'react';
import { createPortal } from 'react-dom';
import styled, { keyframes } from 'styled-components';
import SearchBox, { Backdrop } from './SearchBox';
import SearchResults from './SearchResults';
import { searchBoxRadius } from './config';

interface Props {
  onClose: () => void
  offsetY: number
}

const duration = '0.4s'
const easing = 'cubic-bezier(0.7, -0.4, 0.4, 1.4)'

const enterDialog = keyframes`
  from {
    transform: translateY(calc(var(--offset-y) - var(--margin-top)));
  }

  to {
    transform: translateY(0);
  }
`

const exitDialog = keyframes`
  from {
    transform: translateY(0);
  }

  to {
    transform: translateY(calc(var(--offset-y) - var(--margin-top)));
  }
`

const enterBackdrop = keyframes`
  from {
    opacity: 0;
  }

  to {
    opacity: 1;
  }
`

const exitBackdrop = keyframes`
  from {
    opacity: 1;
  }

  to {
    opacity: 0;
  }
`

const Dialog = styled.dialog<{ offsetY: number }>`
  --margin-top: calc(50vh - 209px);
  --offset-y: ${p => p.offsetY}px;

  outline: none;
  border: none;
  border-radius: ${searchBoxRadius};
  background: transparent;
  margin-top: var(--margin-top);
  padding: 0px;

  animation: ${enterDialog} ${duration} ${easing};

  &::backdrop {
    opacity: 1;
    background: ${color.dialogs.scrimBackground};
    backdrop-filter: blur(4px);
    animation: ${enterBackdrop} ${duration} ${easing};
  }

  &.closing {
    animation: ${exitDialog} ${duration} ${easing};

    &::backdrop {
      animation: ${exitBackdrop} ${duration} ${easing};
    }
  }
`

export default function Component(props: Props) {
  const ref = React.useRef<HTMLDialogElement>(null)

  React.useEffect(() => {
    ref.current?.showModal()
  }, [])

  const doClose = React.useCallback(() => {
    const el = ref.current!
    el.addEventListener('animationend', () => props.onClose());
    el.classList.add('closing');
  }, [])

  React.useEffect(() => {
    const el = ref.current
    if (!el) return

    const closeHandler = (e: CloseEvent) => {
      e.preventDefault()
    }

    el.addEventListener('close', closeHandler)
    return () => {
      el.removeEventListener('close', closeHandler)
    }
  }, [props.onClose])

  React.useEffect(() => {
    setTimeout(() => {
      (ref.current?.querySelector('leo-input') as HTMLElement)?.focus()
    })
  }, [])
  return createPortal(<Dialog ref={ref} offsetY={props.offsetY} onClick={e => {
    // The dropdown uses position: fixed, so it isn't actually inside the dialog
    // bounds, so check if the leo-input contained the click.
    const input = e.nativeEvent.composedPath().find(t => (t as HTMLElement).tagName === 'LEO-INPUT')
    if (input) return

    const rect = e.currentTarget.getBoundingClientRect();

    const clickedInDialog = (
      rect.top <= e.clientY &&
      e.clientY <= rect.top + rect.height &&
      rect.left <= e.clientX &&
      e.clientX <= rect.left + rect.width
    );
    if (!clickedInDialog) {
      doClose()
    }
  }}
  onCancel={e => {
    e.preventDefault()
    doClose()
  }}>
    <Backdrop />
    <SearchBox />
    <SearchResults />
  </Dialog>, document.body)
}
