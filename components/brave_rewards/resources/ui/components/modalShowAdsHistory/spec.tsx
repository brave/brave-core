import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import ModalShowAdsHistory from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('ModalShowAdsHistory tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><ModalShowAdsHistory id='modal' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#modal').length
      expect(assertion).toBe(1)
    })
  })
})
