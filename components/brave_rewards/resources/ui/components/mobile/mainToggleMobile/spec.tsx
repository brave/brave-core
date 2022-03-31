import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import MainToggleMobile from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('MainToggleMobile tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><MainToggleMobile id='mainToggleMobile' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#mainToggleMobile').length
      expect(assertion).toBe(1)
    })
  })
})
