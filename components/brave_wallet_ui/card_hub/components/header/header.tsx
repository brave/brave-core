// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styles
import { HeaderWrapper, BackButton, BackIcon } from './header.style'
import { Text } from '$wallet/components/shared/style'

interface Props {
  onBack?: () => void
  title: string
}

export const Header = (props: Props) => {
  const { onBack, title } = props

  return (
    <HeaderWrapper
      width='100%'
      padding='16px 12px'
      alignItems='center'
    >
      <BackButton
        fab
        kind='plain-faint'
        onClick={onBack}
      >
        <BackIcon name='arrow-left' />
      </BackButton>
      <Text
        variant='default.semibold'
        textColor='primary'
      >
        {title}
      </Text>
    </HeaderWrapper>
  )
}
