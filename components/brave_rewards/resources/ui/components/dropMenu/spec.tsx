/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import { TestThemeProvider } from 'brave-ui/theme'
import DropMenu from './index';

describe('Drop Menu tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><DropMenu id={'dropMenu'} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#dropMenu').length
      expect(assertion).toBe(1)
    })
  })
})
