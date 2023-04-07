// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'

const Kbd = styled.div`
  display: inline-block;
  border-radius: 4px;
  padding: 4px;
  background-color: #f6f8fa;
  border: 1px solid rgba(174, 184, 193, 0.2);
  box-shadow: inset 0 -1px 0 rgba(174, 184, 193, 0.2);
`

export default function Keys({ keys }: { keys: string[] }) {
  return (
    <>
      {keys.map((k, i) => (
        <React.Fragment key={i}>
          {i !== 0 && <span>+</span>}
          <Kbd>{k}</Kbd>
        </React.Fragment>
      ))}
    </>
  )
}
