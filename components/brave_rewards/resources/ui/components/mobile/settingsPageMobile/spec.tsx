/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import SettingsPageMobile from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('SettingsPageMobile tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><SettingsPageMobile id='page' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#page').length
      expect(assertion).toBe(1)
    })
  })
})
