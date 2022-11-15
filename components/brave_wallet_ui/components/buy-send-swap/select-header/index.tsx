// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import {
  Header,
  HeaderText,
  Button,
  BackIcon,
  HeaderSpacing,
  PlusIcon
} from './style'

export interface Props {
  title: string
  hasAddButton?: boolean
  onClickAdd?: () => void
  onBack?: () => void
}

function SelectHeader (props: Props) {
  const { onBack, title, hasAddButton, onClickAdd } = props
  return (
    <Header>
      {onBack
        ? <Button onClick={onBack}><BackIcon /></Button>
        : <HeaderSpacing />
      }
      <HeaderText>{title}</HeaderText>
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

export default SelectHeader
