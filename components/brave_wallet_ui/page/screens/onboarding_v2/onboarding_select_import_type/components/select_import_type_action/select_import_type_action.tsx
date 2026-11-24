// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'

// Styled Components
import {
  ActionIcon,
  ActionWrapper,
  IconsWrapper,
  ArrowIconWrapper,
  ActionText,
} from './select_import_type_action.style'
import { Column, Row } from '$wallet/components/shared/style'

interface Props {
  title: string
  description: string
  icons: Array<string | React.ReactNode>
  onClick?: () => void
}

export const SelectImportTypeAction = ({
  title,
  description,
  icons,
  onClick,
}: Props) => {
  return (
    <ActionWrapper onClick={onClick}>
      <Column width='100%'>
        <Row
          justifyContent='space-between'
          alignItems='center'
        >
          <ActionText
            variant='heading.h3'
            textColor='primary'
            textAlign='left'
          >
            {title}
          </ActionText>
          <ArrowIconWrapper>
            <Icon name='arrow-right' />
          </ArrowIconWrapper>
        </Row>
        <ActionText
          variant='default.regular'
          textColor='secondary'
          textAlign='left'
        >
          {description}
        </ActionText>
      </Column>
      <IconsWrapper>
        {icons.map((icon) =>
          typeof icon === 'string' ? (
            <ActionIcon
              name={icon}
              key={icon}
            />
          ) : (
            icon
          ),
        )}
      </IconsWrapper>
    </ActionWrapper>
  )
}
