// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

export default class TrashIcon extends React.PureComponent {
  render () {
    return (
      <svg width={'16'} height={'16'} xmlns={'https://www.w3.org/2000/svg'}>
        <path d={'M14.0007 4H13.274L12.0707 14.7447C11.992 15.4487 11.3753 16 10.6673 16H5.33398C4.62465 16 4.00932 15.448 3.93132 14.7433L2.74732 4H2.00065C1.63265 4 1.33398 3.70133 1.33398 3.33333C1.33398 2.96533 1.63265 2.66667 2.00065 2.66667H5.33398V2C5.33398 0.897333 6.23132 0 7.33398 0H8.66732C9.76998 0 10.6673 0.897333 10.6673 2V2.66667H14.0007C14.3687 2.66667 14.6673 2.96533 14.6673 3.33333C14.6673 3.70133 14.3687 4 14.0007 4ZM9.33398 2C9.33398 1.63267 9.03465 1.33333 8.66732 1.33333H7.33398C6.96665 1.33333 6.66732 1.63267 6.66732 2V2.66667H9.33398V2ZM4.08865 4L5.25732 14.598C5.25998 14.624 5.30732 14.6667 5.33398 14.6667H10.6673C10.694 14.6667 10.7427 14.6233 10.746 14.5967L11.9327 4H4.08865Z'} fillRule={'evenodd'} />
      </svg>
    )
  }
}
