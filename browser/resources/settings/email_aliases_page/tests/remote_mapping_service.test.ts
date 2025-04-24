// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RemoteMappingService } from '../content/remote_mapping_service'

describe('RemoteMappingService', () => {
  let service: RemoteMappingService

  beforeEach(() => {
    service = new RemoteMappingService()
  })

  describe('getAccountEmail', () => {
    test('returns mock email address', async () => {
      const email = await service.getAccountEmail()
      expect(email).toBe('account-email@gmail.com')
    })
  })

  describe('getAliases', () => {
    test('returns mock aliases', async () => {
      const aliases = await service.getAliases()
      expect(aliases).toHaveLength(2)
      expect(aliases[0]).toEqual({
        email: 'mock-alias1@sandbox.bravealias.com',
        note: 'Mock Alias 1',
        status: true
      })
      expect(aliases[1]).toEqual({
        email: 'mock-alias2@sandbox.bravealias.com',
        note: 'Mock Alias 2',
        status: false
      })
    })
  })

  describe('generateAlias', () => {
    test('generates mock alias with correct format', async () => {
      const alias = await service.generateAlias()
      expect(alias).toMatch(/^mock-\d{4}@sandbox\.bravealias\.com$/)
    })

    test('generates unique aliases', async () => {
      const alias1 = await service.generateAlias()
      const alias2 = await service.generateAlias()
      expect(alias1).not.toBe(alias2)
    })
  })
})
