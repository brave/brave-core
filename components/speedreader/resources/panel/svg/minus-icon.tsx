// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const SvgComponent = (props: any) => (
  <svg
    width={24}
    height={24}
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    {...props}
  >
    <path
      d="M19.01 11H4.99c-.533 0-.99.461-.99 1 0 .539.457 1 .99 1h14.096c.457-.077.914-.461.914-1 0-.539-.457-1-.99-1Z"
      fill="currentColor"
    />
  </svg>
)

export default SvgComponent
