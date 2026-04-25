// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

// Styled Components
import { Column, Text } from '../../../../../shared/style'

interface Props {
  text: string
  icon: string
  onClick: () => void
}

export const PortfolioAssetActionButton = (props: Props) => {
  const { text, icon, onClick } = props

  return (
    <Column gap='4px'>
      <Button
        fab={true}
        onClick={onClick}
      >
        <Icon name={icon} />
      </Button>
      <Text
        textSize='12px'
        isBold={true}
        textColor='primary'
      >
        {text}
      </Text>
    </Column>
  )
}
