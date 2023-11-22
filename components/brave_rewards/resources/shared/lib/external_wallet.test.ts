/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { externalWalletFromExtensionData } from './external_wallet'
import * as mojom from '../../shared/lib/mojom'

describe('external_wallet', () => {
  const convert = externalWalletFromExtensionData

  const basicObject = {
    type: 'uphold',
    status: 2
  }

  describe('externalWalletFromExtensionData', () => {
    it('returns an ExternalWallet for basic data', () => {
      expect(convert(basicObject)).toEqual({
        provider: 'uphold',
        status: mojom.WalletStatus.kConnected,
        username: '',
        links: {
          account: ''
        }
      })
    })

    it('returns null for non-objects', () => {
      expect(convert(123)).toStrictEqual(null)
      expect(convert(null)).toStrictEqual(null)
      expect(convert(undefined)).toStrictEqual(null)
      expect(convert('abc')).toStrictEqual(null)
    })

    it('returns null if property "type" is invalid', () => {
      expect(convert({ ...basicObject, type: undefined })).toStrictEqual(null)
      expect(convert({ ...basicObject, type: 'abc' })).toStrictEqual(null)
      expect(convert({ ...basicObject, type: 123 })).toStrictEqual(null)
    })

    it('returns null if property "status" is invalid', () => {
      expect(convert({ ...basicObject, status: undefined })).toStrictEqual(null)
      expect(convert({ ...basicObject, status: -1 })).toStrictEqual(null)
      expect(convert({ ...basicObject, status: 6 })).toStrictEqual(null)
    })

    it('maps wallet status integers correctly', () => {
      expect(convert({ ...basicObject, status: 2 })).toMatchObject({
        status: mojom.WalletStatus.kConnected
      })

      expect(convert({ ...basicObject, status: 4 })).toMatchObject({
        status: mojom.WalletStatus.kLoggedOut
      })
    })

    it('returns the username', () => {
      expect(convert({ ...basicObject, userName: 'Bob' })).toMatchObject({
        username: 'Bob'
      })
    })

    it('returns links specified on input', () => {
      expect(convert({ ...basicObject, accountUrl: 'url' })).toMatchObject({
        links: {
          account: 'url'
        }
      })
    })
  })
})
