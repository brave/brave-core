/* This Source Code Form is subject to the terms of the Mozilla Public
* License. v. 2.0. If a copy of the MPL was not distributed with this file.
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  lightWidget?: boolean
}

export default class EllipsisIcon extends React.Component<Props, {}> {
  render () {
    const fillColor = this.props.lightWidget ? '#495057' : '#ffffff'
    return (
      <svg xmlns={'http://www.w3.org/2000/svg'} width={'24'} height={'24'}>
        <path d={'M18 14.25a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5zm-6 0a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5zm-6 0a2.25 2.25 0 110-4.5 2.25 2.25 0 010 4.5z'} fill={fillColor} fillRule={'evenodd'}/>
      </svg>
    )
  }
}
