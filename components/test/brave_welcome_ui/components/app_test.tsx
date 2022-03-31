import * as React from 'react'
import { shallow } from 'enzyme'
import { welcomeInitialState } from '../../testData'
import * as storage from '../../../brave_welcome_ui/storage'
import {
  WelcomePage,
  mapStateToProps
} from '../../../../components/brave_welcome_ui/containers/app'

describe('welcomePage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(welcomeInitialState)).toEqual({
        welcomeData: storage.defaultState
      })
    })
  })

  describe('welcomePage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <WelcomePage
          actions={{}}
          welcomeData={welcomeInitialState.welcomeData as Welcome.State}
        />
      )
      const assertion = wrapper.find('#welcomePage')
      expect(assertion.length).toBe(1)
    })
  })
})
