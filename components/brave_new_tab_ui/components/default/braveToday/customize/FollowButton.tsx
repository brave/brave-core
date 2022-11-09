// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import Button, { ButtonProps } from '$web-components/button'
import * as React from 'react'
import styled, { css } from 'styled-components'
import { getLocale } from '$web-common/locale'

interface Props extends Omit<ButtonProps, 'isPrimary'> {
  className?: string
  following: boolean
}

const StyledButton = styled.div<{ following: boolean }>`
  &> button {
    --background: #FFF;
    padding: 5px 14px;
    ${p => p.following && css`
      color: var(--interactive5);
      --inner-border-color: var(--interactive5);
      --outer-border-color: var(--interactive5);
    `}
  }
`

export default function FollowButton (props: Props) {
  const { following, className, ...rest } = props
  return <StyledButton className={className} following={following}>
    <Button {...rest} scale='tiny' isPrimary={!following}>
      {following
        ? getLocale('braveNewsFollowButtonFollowing')
        : getLocale('braveNewsFollowButtonNotFollowing')
      }
    </Button>
  </StyledButton>
}
