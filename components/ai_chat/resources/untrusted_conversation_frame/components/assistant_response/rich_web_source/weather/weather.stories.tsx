import 'react'
import WeatherShowcase from './example'
import { Meta, StoryObj } from '@storybook/react'

const meta: Meta<typeof WeatherShowcase> = {
  title: 'Weather/WeatherShowcase',
  component: WeatherShowcase,
}

export default meta
export const Default: StoryObj<typeof WeatherShowcase> = {}
