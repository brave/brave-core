/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/
import * as React from 'react'

module.exports = (color: string = '#A1A8F2') => (
  <svg width='12' height='11' xmlns='http://www.w3.org/2000/svg'>
    <path
      d='M10.425 3.106h-.004l-.114-.23C9.277 1.023 7.615 0 5.621 0 2.52 0 0 2.465 0 5.504 0 8.534 2.521 11 5.628 11a5.67 5.67 0 0 0 3.275-1.029.452.452 0 0 0 .107-.64.474.474 0 0 0-.654-.104 4.73 4.73 0 0 1-2.735.856C3.039 10.083.937 8.03.937 5.496c0-2.525 2.102-4.58 4.691-4.58 2.086 0 3.249 1.305 3.845 2.377l.047.096c-.404.278-.67.737-.67 1.257 0 .85.705 1.54 1.575 1.54S12 5.496 12 4.646c0-.851-.705-1.54-1.575-1.54'
      fill={color}
      fillRule='evenodd'
    />
  </svg>
)
