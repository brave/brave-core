/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import ModalBackupRestore from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('ModalBackupRestore tests', () => {
  const doNothing = () => { console.log('nothing') }
  const baseComponent = (props?: object) => (
    <TestThemeProvider>
      <ModalBackupRestore
        id='modal'
        activeTabId={0}
        recoveryKey={''}
        onClose={doNothing}
        onCopy={doNothing}
        onPrint={doNothing}
        onSaveFile={doNothing}
        onRestore={doNothing}
        onImport={doNothing}
        onTabChange={doNothing}
        {...props}
      />
    </TestThemeProvider>
  )

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
