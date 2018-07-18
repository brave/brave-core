import * as React from 'react'
import { shallow } from 'enzyme'
import {
  WelcomePage,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_welcome_ui/components/app'
import { types } from '../../../brave_welcome_ui/constants/welcome_types'
let appState: Welcome.ApplicationState = {
  welcomeData: {
    pageIndex: 0
  }
}

let welcomeData: Welcome.State = {
  pageIndex: 0
}

describe('WelcomePage component', () => {
  describe('mapStateToProps', () => {
    it('should show the current pageIndex', () => {
      expect(mapStateToProps(appState).welcomeData).toEqual({ pageIndex: 0 })
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

  describe('welcomePage component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <WelcomePage
          actions={{}}
          welcomeData={welcomeData}
        />
      )
      const assertion = wrapper.find('#welcomePage')
      expect(assertion.length).toBe(1)
    })
  })
})