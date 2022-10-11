// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const SvgComponent = (props: any) => (
  <svg
    width={16}
    height={16}
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    {...props}
  >
    <path
      d="M7 7V.99C7 .457 7.462 0 8 0s.923.457 1 .914V7h6.01c.533 0 .99.462.99 1s-.457.923-.914 1H9v6.01c0 .533-.462.99-1 .99s-1-.457-1-.99V9H.99C.457 9 0 8.538 0 8s.457-1 .99-1H7Z"
      fill="currentColor"
    />
  </svg>
)

export default SvgComponent
