// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { StoryFn, Meta } from '@storybook/react'
import * as Icons from 'brave-ui/components/icons'
import Button, { ButtonIconContainer } from './index'

export default {
  title: 'Button',
  component: Button,
  argTypes: {
    isPrimary: {
      type: 'boolean'
    },
    isTertiary: {
      type: 'boolean'
    },
    isLoading: {
      type: 'boolean'
    },
    isDisabled: {
      type: 'boolean'
    },
    scale: {
      options: ['regular', 'tiny', 'small', 'large', 'jumbo'],
      control: { type: 'select' }
    }
  }
} as Meta<typeof Button>

const Template: StoryFn<typeof Button> = function (args) {
  return (
    <div style={{ display: 'flex', gap: '16px', alignItems: 'flex-start' }}>
      <Button {...args}>I am a button</Button>
      <Button {...args}>
        <ButtonIconContainer>
          <Icons.PlusIcon />
        </ButtonIconContainer>
        I am a button with icon
      </Button>
      <Button {...args}>
        <ButtonIconContainer>
          <Icons.PlusIcon />
        </ButtonIconContainer>
      </Button>
    </div>
  )
}

export const Everything = {
  render: Template
}
