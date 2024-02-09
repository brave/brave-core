// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button, { ButtonProps } from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as React from 'react'
import styled from 'styled-components'

const StyledButton = styled(Button)`
  --leo-button-color: rgba(150, 150, 150, 0.8);
  backdrop-filter: blur(64px);
`

export default function FollowButton(props: ButtonProps<undefined, boolean> & { following: boolean }) {
  const { following, ...rest } = props
  return <StyledButton {...rest} fab size='tiny'>
    <Icon name={following ? 'minus' : 'plus-add'} />
  </StyledButton>
}
