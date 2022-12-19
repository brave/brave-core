// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const SvgComponent = (props: any) => (
  <svg
    viewBox="0 0 22 18"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    {...props}
  >
    <path
      fillRule="evenodd"
      clipRule="evenodd"
      d="M2.971 8.083h18.112a.917.917 0 1 1 0 1.834H2.971l6.222 5.743a.917.917 0 0 1 .052 1.295.917.917 0 0 1-1.295.052L.49 10.123a1.528 1.528 0 0 1 0-2.246L7.95.993A.917.917 0 0 1 9.194 2.34L2.971 8.083Z"
      fill="#F0F2FF"
    />
  </svg>
)

export default SvgComponent
