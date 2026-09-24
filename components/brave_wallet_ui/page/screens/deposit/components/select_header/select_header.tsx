// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  Header,
  Button,
  BackIcon,
  HeaderSpacing,
  PlusIcon,
} from './select_header.style'
import { Text } from '../../../../../components/shared/style'

export interface Props {
  title: string
  hasAddButton?: boolean
  onClickAdd?: () => void
  onBack?: () => void
}

export const SelectHeader = (props: Props) => {
  const { onBack, title, hasAddButton, onClickAdd } = props
  return (
    <Header>
      {onBack ? (
        <Button onClick={onBack}>
          <BackIcon />
        </Button>
      ) : (
        <HeaderSpacing />
      )}
      <Text
        textColor='primary'
        variant='default.semibold'
      >
        {title}
      </Text>
      {hasAddButton ? (
        <Button onClick={onClickAdd}>
          <PlusIcon />
        </Button>
      ) : (
        <HeaderSpacing />
      )}
    </Header>
  )
}
