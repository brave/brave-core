/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState } from '../lib/app_model_context'

export function GeneralInfo() {
  const environment = useAppState((state) => state.environment)
  const isKeyInfoSeedValid = useAppState((state) => state.isKeyInfoSeedValid)
  const paymentId = useAppState((state) => state.paymentId)
  const createdAt = useAppState((state) => state.createdAt)
  const creationEnvironment = useAppState((state) => state.creationEnvironment)
  const declaredGeo = useAppState((state) => state.declaredGeo)
  const balance = useAppState((state) => state.balance)
  const externalWallet = useAppState((state) => state.externalWallet)
  const externalWalletId = useAppState((state) => state.externalWalletId)
  const accountId = useAppState((state) => state.externalWalletAccountId)

  function connectionState() {
    if (!externalWallet) {
      return 'Not connected'
    }
    if (!externalWallet.authenticated) {
      return 'Logged out'
    }
    return 'Connected'
  }

  return (
    <>
      <div className='content-card'>
        <h4>Rewards Info</h4>
        <section className='key-value-list'>
          <div>
            <span>Key info seed:</span>
            <span>
              {creationEnvironment
                ? isKeyInfoSeedValid
                  ? 'Valid'
                  : 'Invalid'
                : 'Not created'}
            </span>
          </div>
          <div>
            <span>Rewards payment ID:</span>
            <span>{paymentId}</span>
          </div>
          <div>
            <span>Rewards profile created at:</span>
            <span>
              {createdAt.hasValue()
                ? new Date(createdAt.value()).toISOString()
                : null}
            </span>
          </div>
          <div>
            <span>Creation environment:</span>
            <span>{creationEnvironment}</span>
          </div>
          <div>
            <span>Current environment:</span>
            <span>{creationEnvironment ? environment : null}</span>
          </div>
          <div>
            <span>Rewards country:</span>
            <span>{declaredGeo}</span>
          </div>
        </section>
      </div>
      <div className='content-card'>
        <h4>Balance Info</h4>
        <section className='key-value-list'>
          <div>
            <span>Total balance:</span>
            <span>
              {balance.hasValue() ? `${balance.value()} BAT` : 'Loading...'}
            </span>
          </div>
        </section>
      </div>
      <div className='content-card'>
        <h4>Connected Account Info</h4>
        <section className='key-value-list'>
          <div>
            <span>Rewards state:</span>
            <span>{connectionState()}</span>
          </div>
          <div>
            <span>Account deposit address:</span>
            <span>{externalWalletId}</span>
          </div>
          <div>
            <span>Account type:</span>
            <span>{externalWallet?.provider}</span>
          </div>
          <div>
            <span>Account provider ID:</span>
            <span>{accountId || externalWalletId}</span>
          </div>
        </section>
      </div>
    </>
  )
}
