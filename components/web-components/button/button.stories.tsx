import React from 'react'
import { ComponentStory, ComponentMeta } from '@storybook/react'
import { PlusIcon } from 'brave-ui/components/icons'
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
} as ComponentMeta<typeof Button>

const Template: ComponentStory<typeof Button> = function (args) {
  return <div style={{ display: 'flex', gap: '16px', alignItems: 'flex-start' }}>
    <Button {...args}>I am a button</Button>
    <Button {...args}>
      <ButtonIconContainer>
        <PlusIcon />
      </ButtonIconContainer>
      I am a button with icon
    </Button>
    <Button {...args}>
      <ButtonIconContainer>
        <PlusIcon />
      </ButtonIconContainer>
    </Button>
  </div>
}

export const Everything = Template.bind({})
