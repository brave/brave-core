// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { WalletState } from '../../constants/types'
import { WalletActions } from '../actions'
import { useApiProxy } from './use-api-proxy'

const MAX_ATTEMPTS = 3 // The amount of tries before locking the wallet

/**
 * Provides a methods to check the user's password,
 * and lock the wallet after too many incorrect attempts
 *
 * Uses the context-injected ApiProxy keyring
 * Uses redux to track attempts globally
 */
export const usePasswordAttempts = () => {
  // custom hooks
  const { keyringService } = useApiProxy()

  // redux
  const dispatch = useDispatch()
  const attempts = useSelector(({ wallet }: { wallet: WalletState }) => {
    return wallet.passwordAttempts
  })

  // methods
  const attemptPasswordEntry = React.useCallback(async (password: string): Promise<boolean> => {
    if (!password) { // require password to view key
      return false
    }

    // entered password must be correct
    const {
      result: isPasswordValid
    } = await keyringService.validatePassword(password)

    if (!isPasswordValid) {
      const newAttempts = attempts + 1
      if (newAttempts >= MAX_ATTEMPTS) {
        // lock wallet
        keyringService.lock()
        dispatch(WalletActions.setPasswordAttempts(0)) // reset attempts now that the wallet is locked
        return false
      }

      // increase attempts count
      dispatch(WalletActions.setPasswordAttempts(newAttempts))
      return false
    }

    // correct password entered, reset attempts
    dispatch(WalletActions.setPasswordAttempts(0))
    return isPasswordValid
  }, [keyringService, attempts])

  return {
    attemptPasswordEntry,
    attempts
  }
}
