/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export default class GeminiLogo extends React.PureComponent {

  render () {
    return (
      <svg width={'24'} height={'24'} fill={'none'} xmlns={'http://www.w3.org/2000/svg'}>
        <g clipPath={'url(#clip0)'}>
          <path d={'M15.684 0c-4.222 0-7.806 3.247-8.258 7.426C3.247 7.878 0 11.464 0 15.684A8.321 8.321 0 008.316 24c4.222 0 7.82-3.247 8.258-7.426C20.753 16.122 24 12.536 24 8.316A8.322 8.322 0 0015.684 0zm6.37 9.247a6.47 6.47 0 01-5.424 5.42v-5.42h5.425zM1.949 14.753A6.47 6.47 0 017.37 9.318v5.423H1.948v.012zm12.734 1.877a6.437 6.437 0 01-12.734 0h12.734zm.07-7.383v5.494H9.248V9.247h5.506zm7.3-1.877H9.318a6.437 6.437 0 0112.734 0z'} fill={'#00DCFA'}/>
        </g>
        <defs>
          <clipPath id={'clip0'}>
            <path fill={'#fff'} d={'M0 0h24v24H0z'}/>
          </clipPath>
        </defs>
      </svg>
    )
  }
}
