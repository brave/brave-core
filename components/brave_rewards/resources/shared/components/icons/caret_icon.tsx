/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  direction: 'up' | 'down' | 'left' | 'right'
}

function rotation (direction: string) {
  switch (direction) {
    case 'up': return 180
    case 'left': return 90
    case 'right': return -90
    default: return 0
  }
}

export function CaretIcon (props: Props) {
  return (
    <svg
      viewBox='0 0 32 32'
      className='icon'
      style={{ transform: `rotate(${rotation(props.direction)}deg)` }}
    >
      <path d='M16 19.047l11.04-9.2a1.5 1.5 0 0 1 1.92 2.305l-12 10a1.5 1.5 0 0 1-1.92 0l-12-10a1.5 1.5 0 1 1 1.92-2.304l11.04 9.2z' />
    </svg>
  )
}
