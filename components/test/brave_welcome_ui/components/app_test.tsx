import * as React from 'react'
import { shallow } from 'enzyme'
import { types } from '../../../brave_welcome_ui/constants/welcome_types'
import { welcomeInitialState } from '../../testData'
import {
  WelcomePage,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_welcome_ui/components/app'

describe('welcomePage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(welcomeInitialState)).toEqual({
        welcomeData: {
          pageIndex: 0
        }
      })
    })
  })

  describe('mapDispatchToProps', () => {
    it('should change the current pageIndex if goToPageRequested is fired', () => {
      const dispatch = jest.fn()

      // For the `mapDispatchToProps`, call it directly but pass in
      // a mock function and check the arguments passed in are as expected
      mapDispatchToProps(dispatch).actions.goToPageRequested(1337)
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.GO_TO_PAGE_REQUESTED,
        meta: undefined,
        payload: { pageIndex: 1337 }
      })
    })
  })

  describe('welcomePage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <WelcomePage
          actions={{}}
          welcomeData={welcomeInitialState.welcomeData}
        />
      )
      const assertion = wrapper.find('#welcomePage')
      expect(assertion.length).toBe(1)
    })
  })
})