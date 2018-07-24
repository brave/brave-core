/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/
import * as React from 'react'

module.exports = (color: string) => (
  <svg width='22' height='21' xmlns='http://www.w3.org/2000/svg'>
    <path
      d='M11.248 13.348L2.84 17.23l-1.2-14.418 9.607 10.536zm8.214-3.794l-6.818 3.148-9.088-9.968 15.906 6.82zm-7.73 8.182l-1.769-2.178 1.769-.817v2.995zm-10.265.66A.73.73 0 0 0 2.506 19l6.083-2.81 3.307 4.075a.731.731 0 0 0 .814.23.733.733 0 0 0 .489-.692v-5.74l8.374-3.867a.736.736 0 0 0-.017-1.342L1.036.056 1.018.048C1.008.044.995.048.984.044A.728.728 0 0 0 .688 0C.658.003.631.01.602.015a.694.694 0 0 0-.238.09C.344.117.32.115.3.13.292.136.288.149.279.157.266.169.249.172.236.183a.717.717 0 0 0-.23.567C.008.763-.001.773 0 .787l1.467 17.61z'
      fill={color}
      fillRule='evenodd'
    />
  </svg>
)
