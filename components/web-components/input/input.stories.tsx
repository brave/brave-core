import React from 'react'
import { ComponentStory, ComponentMeta } from '@storybook/react'
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
} as ComponentMeta<typeof Component>

const Template: ComponentStory<typeof Component> = function (args, o) {
  const [value, setValue] = React.useState('I am an input')
  const handleChange: React.FormEventHandler<HTMLInputElement> = (e) => {
    setValue(e.currentTarget.value)
  }
  return <div style={{ display: 'flex', gap: '16px', alignItems: 'flex-start' }}>
    <Component {...args} value={value} onChange={handleChange} />
  </div>
}

export const Everything = Template.bind({})
