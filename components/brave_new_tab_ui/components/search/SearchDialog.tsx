// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import styled, { keyframes } from 'styled-components'
import SearchBox from './SearchBox';
import SearchResults from './SearchResults';
import { spacing } from '@brave/leo/tokens/css';

interface Props {
  onClose: () => void
  offsetY: number
}

const duration = '0.12s'

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
    background: transparent;
    backdrop-filter: blur(0);
  }

  to {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(64px);
  }
`

const exitBackdrop = keyframes`
  from {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(64px);
  }

  to {
    background: transparent;
    backdrop-filter: blur(0);
  }
`

const Dialog = styled.dialog<{ offsetY: number }>`
  --margin-top: ${spacing['9Xl']};
  --offset-y: ${p => p.offsetY}px;

  outline: none;
  border: none;
  background: transparent;
  margin-top: var(--margin-top);
  padding: 2px;

  animation: ${enterDialog} ${duration} ease-in-out;

  &::backdrop {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(64px);
    animation: ${enterBackdrop} ${duration} ease-in-out;
  }

  &.closing {
    animation: ${exitDialog} ${duration} ease-in-out;

    &::backdrop {
      animation: ${exitBackdrop} ${duration} ease-in-out;
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
    el.addEventListener('cancel', e => e.preventDefault())
    return () => {
      el.removeEventListener('close', closeHandler)
      el.removeEventListener('cancel', closeHandler)
    }
  }, [props.onClose])

  React.useEffect(() => {
    setTimeout(() => {
      (ref.current?.querySelector('leo-input') as HTMLElement)?.focus()
    })
  }, [])
  return <Dialog ref={ref} offsetY={props.offsetY} onClick={e => {
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
  }}>
    <SearchBox />
    <SearchResults />
  </Dialog>
}
