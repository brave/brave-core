// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Styles
import { Button } from './action_buttion.style'
import { Column, Text } from '$wallet/components/shared/style'

interface Props {
  icon: string
  label: string
  onClick: () => void
}

export const ActionButton = (props: Props) => {
  const { icon, label, onClick } = props

  return (
    <Column gap='4px'>
      <Button
        size='large'
        onClick={onClick}
        fab
      >
        <Icon name={icon} />
      </Button>
      <Text
        variant='small.regular'
        textColor='primary'
      >
        {label}
      </Text>
    </Column>
  )
}
