import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import MainToggle from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('MainToggle tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><MainToggle id='mainToggle' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#mainToggle').length
      expect(assertion).toBe(1)
    })
  })
})
