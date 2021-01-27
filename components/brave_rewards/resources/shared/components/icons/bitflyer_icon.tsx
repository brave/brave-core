/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  white?: boolean
}

export function BitflyerIcon (props: Props) {
  const fill = props.white ? '#fff' : '#468ccb'
  return (
    <svg className='icon' xmlns='http://www.w3.org/2000/svg' viewBox='-5 0 45.13 37.13'>
      <path fill={fill} d='M0 0h11.23v11.23H0zm12.95 0h11.23v11.23H12.95zM0 12.95h11.23v11.23H0zm0 12.94h11.23v11.23H0z' />
      <circle cx='18.56' cy='18.56' r='5.57' fill='#ee7f4a' />
      <path fill={fill} d='M25.89 0h11.23v11.23H25.89z' />
    </svg>
  )
}
