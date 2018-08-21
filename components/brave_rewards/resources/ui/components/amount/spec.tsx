/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Amount from './index'
import { TestThemeProvider } from '../../../theme'

describe('Amount tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><Amount id='amount' amount={1} converted={0.4} onSelect={() => {}} {...props}  /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#amount').length
      expect(assertion).toBe(1)
    })
  })
})
