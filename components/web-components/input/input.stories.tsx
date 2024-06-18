// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { Meta, StoryFn } from '@storybook/react'
import Component from './index'

export default {
  title: 'Input',
  component: Component,
  argTypes: {
    isRequired: {
      type: 'boolean'
    },
    isErrorAlwaysShown: {
      type: 'boolean'
    },
    errorMessage: {
      type: 'string'
    },
    disabled: {
      type: 'boolean'
    },
    label: {
      type: 'string'
    }
    // scale: {
    //   options: ['regular', 'tiny', 'small', 'large', 'jumbo'],
    //   control: { type: 'select' }
    // }
  }
} as Meta<typeof Component>

const Template: StoryFn<typeof Component> = function (args, o) {
  const [value, setValue] = React.useState('I am an input')
  const handleChange: React.FormEventHandler<HTMLInputElement> = (e) => {
    setValue(e.currentTarget.value)
  }
  return (
    <div style={{ display: 'flex', gap: '16px', alignItems: 'flex-start' }}>
      <Component
        {...args}
        value={value}
        onChange={handleChange}
      />
    </div>
  )
}

export const Everything = {
  render: Template
}
