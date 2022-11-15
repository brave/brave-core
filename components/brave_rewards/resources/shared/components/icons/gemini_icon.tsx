/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export function GeminiIcon () {
  const fill = 'var(--provider-icon-color, #00DCFA)'
  return (
    <svg className='icon' fill='none' viewBox='0 0 26 26'>
      <path fill={fill} fillRule='evenodd' clipRule='evenodd' d='M8.49706 8.27587C8.96774 3.92324 12.7015.540527 17.0997.540527c4.7825.003405 8.6586 3.879533 8.662 8.662033 0 4.39664-3.3827 8.13194-7.7353 8.60264-.456 4.3526-4.2045 7.7353-8.60264 7.7353-4.7825-.0034-8.658637-3.8795-8.662041-8.662 0-4.3966 3.382711-8.13195 7.735341-8.60263zm9.60334 7.58723c2.913-.4429 5.2-2.7299 5.6452-5.6452h-5.6452v5.6452zm-15.32255 0l.00186-.0123h5.6433v-5.6329c-2.91423.4441-5.20008 2.7265-5.6433 5.6329h-.00186v.0123zm6.65322 7.6613c3.33283 0 6.16303-2.4014 6.65323-5.6452H2.77785c.49017 3.2438 3.32043 5.6452 6.65322 5.6452zm6.65323-13.3065v5.6452h-5.6452v-5.6452h5.6452zm-5.6452-2.01608h13.3065c-.4902-3.24373-3.3204-5.64516-6.6532-5.64516s-6.1631 2.40143-6.6533 5.64516z' />
    </svg>
  )
}
