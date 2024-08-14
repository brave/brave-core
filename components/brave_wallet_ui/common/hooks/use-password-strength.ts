// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

export interface PasswordStrengthResults {
  isLongEnough: boolean
}

export const usePasswordStrength = (initialPassword: string = '') => {
  // state
  const [password, setPassword] = React.useState<string>(initialPassword)
  const [confirmedPassword, setConfirmedPassword] =
    React.useState<string>(initialPassword)

  // methods
  const checkIsStrongPassword = React.useCallback((pass: string) => {
    // is at least 8 characters
    // to align with NIST 800-63b R3 section 5.1.1.1
    // which is where NIST defines password requirements
    const isLongEnough = pass.length >= 8

    // granular results of password strength check
    return {
      isLongEnough,
      isStrongPassword: isLongEnough
    }
  }, [])

  // memos
  const passwordStrength: PasswordStrengthResults = React.useMemo(() => {
    return checkIsStrongPassword(password)
  }, [checkIsStrongPassword, password])

  const isStrongPassword = React.useMemo(() => {
    return passwordStrength.isLongEnough
  }, [passwordStrength.isLongEnough])

  const hasPasswordError = React.useMemo(() => {
    if (password === '') {
      return false
    }
    return !isStrongPassword
  }, [password, isStrongPassword])

  const hasConfirmedPasswordError = React.useMemo(() => {
    if (confirmedPassword === '') {
      return false
    } else {
      return confirmedPassword !== password
    }
  }, [confirmedPassword, password])

  // computed
  const passwordsMatch = password === confirmedPassword
  const isValid =
    !(
      hasConfirmedPasswordError ||
      hasPasswordError ||
      password === '' ||
      confirmedPassword === ''
    ) && isStrongPassword

  return {
    confirmedPassword,
    password,
    onPasswordChanged: setPassword,
    setConfirmedPassword,
    isStrongPassword,
    isValid,
    hasConfirmedPasswordError,
    hasPasswordError,
    checkIsStrongPassword,
    passwordStrength,
    passwordsMatch
  }
}
