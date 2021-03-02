// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

export default class CloseIcon extends React.PureComponent {
  render () {
    return (
      <svg width={'16'} height={'16'} xmlns={'https://www.w3.org/2000/svg'}>
        <path d={'M3.2793 1.94622C2.94084 1.60777 2.3921 1.60777 2.05365 1.94622C1.71519 2.28468 1.71519 2.83342 2.05365 3.17188L6.5485 7.66673L2.05365 12.1616C1.71519 12.5 1.71519 13.0488 2.05365 13.3872C2.3921 13.7257 2.94084 13.7257 3.2793 13.3872L7.77415 8.89238L12.1417 13.2599C12.4802 13.5984 13.0289 13.5984 13.3674 13.2599C13.7058 12.9215 13.7058 12.3727 13.3674 12.0343L8.9998 7.66673L13.3674 3.29918C13.7058 2.96073 13.7058 2.41198 13.3674 2.07353C13.0289 1.73508 12.4802 1.73508 12.1417 2.07353L7.77415 6.44108L3.2793 1.94622Z'} fillRule={'evenodd'}/>
      </svg>
    )
  }
}
