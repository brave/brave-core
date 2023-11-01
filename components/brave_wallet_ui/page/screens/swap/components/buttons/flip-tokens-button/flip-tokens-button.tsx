// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Button, ButtonIcon, Wrapper } from './flip-tokens-button.style'

interface Props {
  onClick: () => void
}

export const FlipTokensButton = (props: Props) => {
  const { onClick } = props

  return (
    <Wrapper>
      <Button onClick={onClick}>
        <ButtonIcon
          name='arrow-down'
          size={28}
        />
      </Button>
    </Wrapper>
  )
}
