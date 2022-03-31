import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import TableContribute from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('TableContribute tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><TableContribute id='table' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#table').length
      expect(assertion).toBe(1)
    })
  })
})
