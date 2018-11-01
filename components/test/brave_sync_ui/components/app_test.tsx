/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { types } from '../../../brave_sync/ui/constants/sync_types'
import { syncInitialState } from '../../testData'
import {
  SyncPage,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_sync/ui/components/app'

describe('sync page component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(syncInitialState)).toEqual({
        syncData: {
          devices: [{ id: 0, lastActive: '', name: '' }],
          isSyncConfigured: false,
          seedQRImageSource: '',
          syncWords: '',
          thisDeviceName: ''
        }
      })
    })
  })

  describe('mapDispatchToProps', () => {
    it('should change sync words if onHaveSyncWords is fired', () => {
      const dispatch = jest.fn()

      mapDispatchToProps(dispatch).actions.onHaveSyncWords('a brave pokemon appeared')
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.SYNC_ON_HAVE_SYNC_WORDS,
        meta: undefined,
        payload: { syncWords: 'a brave pokemon appeared' }
      })
    })
  })

  describe('sync page dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <SyncPage
          actions={{}}
          syncData={syncInitialState.syncData as Sync.State}
        />
      )
      const assertion = wrapper.find('#syncPage')
      expect(assertion.length).toBe(1)
    })
  })
})
