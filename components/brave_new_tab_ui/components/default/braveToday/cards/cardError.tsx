// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import * as Card from '../cardIntro'

const Content = styled('div')`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
`

const Heading = styled('h3')`
  margin: 16px 0;
  font-weight: 600;
  font-size: 20px;
`

const Graphic = styled('div')`
  width: 64px;
  height: 46px;
`

function Icon () {
  return (
    <svg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 64 46'>
      <path
        fill='#F0F2FF'
        fillRule='evenodd'
        d='M49.67 46H13.33C5.36 46 0 41.32 0 34.37c0-5.45 3.08-10.33 7.84-12.74l-.02-.84C7.82 9.69 16.92.67 28.1.67a20.3 20.3 0 0118.37 11.65c9.74.4 17.54 8.4 17.54 18.17C64 39.33 57.84 46 49.67 46zm-3.98-28.37c-.24 0-.47.02-.7.05-1.24.14-2.4-.6-2.81-1.77A14.94 14.94 0 0028.08 6a14.88 14.88 0 00-14.92 14.79c0 .68.06 1.37.18 2.11a2.67 2.67 0 01-1.8 2.96 8.95 8.95 0 00-6.2 8.5c0 5.85 6.12 6.3 8 6.3h36.33c4.47 0 9-3.5 9-10.17 0-7.1-5.83-12.86-12.98-12.86zM40 35.33c-.68 0-1.37-.26-1.89-.78-.05-.04-2.06-1.88-6.11-1.88-4.05 0-6.06 1.84-6.14 1.91-1.07 1-2.75.97-3.76-.07a2.65 2.65 0 01.01-3.73c.36-.35 3.63-3.45 9.89-3.45 6.26 0 9.53 3.1 9.89 3.45A2.66 2.66 0 0140 35.33zm-5.33-16H40v5.34h-5.33v-5.34zm-10.67 0h5.33v5.34H24v-5.34z'
        clipRule='evenodd'
      />
    </svg>
  )
}

export default function CardError () {
  return (
    <Card.Intro>
      <Content>
        <Graphic>
          <Icon />
        </Graphic>
        <Heading>
          Oops…
        </Heading>
        <Card.Paragraph>
          Brave Today is experiencing some issues. Try again.
        </Card.Paragraph>
      </Content>
    </Card.Intro>
  )
}
