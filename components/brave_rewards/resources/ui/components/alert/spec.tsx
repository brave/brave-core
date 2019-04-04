/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Alert, { Props } from './index'
import { TestThemeProvider } from '../../../theme'

describe('Alert tests', () => {
  const baseComponent = (props: Props) =>
    <TestThemeProvider>
      <Alert id='alert' {...props} />
    </TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent({ type: 'success' })
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent({ type: 'success' }))
      const assertion = wrapper.find('#alert').length
      expect(assertion).toBe(1)
    })
  })
})
