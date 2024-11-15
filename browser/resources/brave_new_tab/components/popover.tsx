/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  className?: string
  isOpen: boolean
  children: React.ReactNode
  onClose: () => void
}

export function Popover(props: Props) {
  const elementRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    elementRef.current?.setAttribute('popover', 'auto')
  }, [])

  React.useEffect(() => {
    if (props.isOpen) {
      elementRef.current?.showPopover()
    } else {
      elementRef.current?.hidePopover()
    }
  }, [props.isOpen])

  React.useEffect(() => {
    const onToggle = (event: ToggleEvent) => {
      if (event.newState === 'closed') {
        props.onClose()
      }
    }
    const elem = elementRef.current
    elem?.addEventListener('toggle', onToggle)
    return () => elem?.removeEventListener('toggle', onToggle)
  }, [props.onClose])

  return (
    <div ref={elementRef} className={props.className}>
      {props.children}
    </div>
  )
}
