// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  ActionIcon,
  ActionWrapper,
  IconsWrapper,
  RightArrow,
  Subtitle,
  Title
} from '../onboarding-import-wallet-type.style'
import { Row, VerticalSpace } from '../../../../../../components/shared/style'

interface Props {
  title: string
  description: string
  icons: Array<string | React.ReactNode>
  onClick?: () => void
}

export const ImportTypeAction = ({
  title,
  description,
  icons,
  onClick
}: Props) => {
  return (
    <ActionWrapper onClick={onClick}>
      <Row
        justifyContent='space-between'
        alignItems='center'
      >
        <Title>{title}</Title>
        <RightArrow />
      </Row>
      <VerticalSpace space='10px' />
      <Subtitle>{description}</Subtitle>
      <VerticalSpace space='10px' />
      <IconsWrapper>
        {icons.map((icon) =>
          typeof icon === 'string' ? (
            <ActionIcon
              name={icon}
              key={icon}
            />
          ) : (
            icon
          )
        )}
      </IconsWrapper>
    </ActionWrapper>
  )
}
