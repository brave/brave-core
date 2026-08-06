// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button, { ButtonProps } from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'

const StyledButton = styled(Button)`
  --leo-button-color: color-mix(in srgb, ${color.neutral[30]} 80%, transparent);
  backdrop-filter: blur(64px);
`

export default function FollowButton(props: ButtonProps<undefined, boolean, boolean> & { following: boolean }) {
  const { following, ...rest } = props
  return <StyledButton {...rest} fab size='tiny'>
    <Icon name={following ? 'minus' : 'plus-add'} />
  </StyledButton>
}
