// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Styles
import { IconBubble } from '../settings.style'
import { LabelAndIcon, Toggle } from './section_toggle.style'
import { Row, Text } from '$wallet/components/shared/style'

interface Props {
  label: string
  icon: string
  checked: boolean
  onChange: (checked: boolean) => void
}

export const SectionToggle = (props: Props) => {
  const { label, icon, checked, onChange } = props

  return (
    <Row
      padding='12px'
      alignItems='center'
      justifyContent='space-between'
    >
      <Toggle
        size='small'
        checked={checked}
        onChange={(detail) => onChange(detail.checked)}
      >
        <LabelAndIcon
          gap='8px'
          justifyContent='flex-start'
        >
          <IconBubble>
            <Icon name={icon} />
          </IconBubble>
          <Text
            variant='default.semibold'
            textColor='primary'
          >
            {label}
          </Text>
        </LabelAndIcon>
      </Toggle>
    </Row>
  )
}
