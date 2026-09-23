// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ProgressRing from '@brave/leo/react/progressRing'
import { getLocale } from '$web-common/locale'
import '../strings'

export interface BraveOriginHandler {
  checkPurchaseState(): Promise<{ isPurchased: boolean }>
  verifyPurchaseId(
    purchaseId: string,
  ): Promise<{ success: boolean; errorMessage: string }>
  openBuyWindow(): void
  closeDialog(): void
  proceedFree(): void
}

interface AppProps {
  handler: BraveOriginHandler
  isLinuxFreeEligible?: boolean
}

function renderLinuxDescription2(onBuyClick: () => void) {
  const raw = getLocale(S.BRAVE_ORIGIN_STARTUP_LINUX_DESCRIPTION2)
  const parts = raw.split(/\$[12]/)
  if (parts.length !== 3) {
    return <p>{raw}</p>
  }
  return (
    <p>
      {parts[0]}
      <a
        href='#'
        className='purchase-link'
        onClick={(e) => {
          e.preventDefault()
          onBuyClick()
        }}
      >
        {parts[1]}
      </a>
      {parts[2]}
    </p>
  )
}

export function App({ handler, isLinuxFreeEligible }: AppProps) {
  const [currentView, setCurrentView] = React.useState<'main' | 'restore'>(
    'main',
  )
  const [purchaseId, setPurchaseId] = React.useState('')
  const [verifying, setVerifying] = React.useState(false)
  const [error, setError] = React.useState('')
  const inputRef = React.useRef<HTMLInputElement>(null)

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        handler.checkPurchaseState().then(({ isPurchased }) => {
          if (isPurchased) {
            handler.closeDialog()
          }
        })
      }
    }
    document.addEventListener('visibilitychange', onVisibilityChange)
    return () =>
      document.removeEventListener('visibilitychange', onVisibilityChange)
  }, [handler])

  const focusInput = () => {
    requestAnimationFrame(() => inputRef.current?.focus())
  }

  const onRestoreClick = () => {
    setCurrentView('restore')
    setError('')
    setPurchaseId('')
    focusInput()
  }

  const onBuyClick = () => {
    handler.openBuyWindow()
    setCurrentView('restore')
    setError('')
    setPurchaseId('')
    focusInput()
  }

  const onVerifyClick = async () => {
    if (!purchaseId || verifying) {
      return
    }
    setVerifying(true)
    setError('')
    const { success, errorMessage } = await handler.verifyPurchaseId(purchaseId)
    setVerifying(false)
    if (success) {
      handler.closeDialog()
    } else {
      setError(errorMessage)
    }
  }

  if (currentView === 'restore') {
    return (
      <div
        className='brave-origin-startup'
        data-verifying={verifying || undefined}
      >
        <div className='container'>
          <img
            src='/assets/Lion.svg'
            className='logo'
            alt=''
          />
          <h1>{getLocale(S.BRAVE_ORIGIN_STARTUP_RESTORE_TITLE)}</h1>
          <div className='description'>
            <p>{getLocale(S.BRAVE_ORIGIN_STARTUP_RESTORE_DESCRIPTION)}</p>
          </div>
          <div className='restore-container'>
            <div className='input-group'>
              <label>
                {getLocale(S.BRAVE_ORIGIN_STARTUP_PURCHASE_ID_LABEL)}
              </label>
              <input
                ref={inputRef}
                type='text'
                value={purchaseId}
                onChange={(e) => {
                  setPurchaseId(e.target.value.trim())
                  setError('')
                }}
                placeholder={getLocale(
                  S.BRAVE_ORIGIN_STARTUP_PURCHASE_ID_PLACEHOLDER,
                )}
                disabled={verifying}
              />
              {error && <div className='error-message'>{error}</div>}
              {verifying && (
                <div className='verifying-message'>
                  <ProgressRing />
                  {getLocale(S.BRAVE_ORIGIN_STARTUP_VERIFYING_MESSAGE)}
                </div>
              )}
            </div>
            <div className='buttons'>
              <Button
                kind='outline'
                onClick={onVerifyClick}
                isDisabled={verifying || !purchaseId}
              >
                {verifying
                  ? getLocale(S.BRAVE_ORIGIN_STARTUP_VERIFYING_MESSAGE)
                  : getLocale(S.BRAVE_ORIGIN_STARTUP_VERIFY_BUTTON)}
              </Button>
              <Button
                kind='plain-faint'
                onClick={onBuyClick}
              >
                {getLocale(S.BRAVE_ORIGIN_STARTUP_BUY_BUTTON)}
              </Button>
            </div>
          </div>
        </div>
      </div>
    )
  }

  if (isLinuxFreeEligible) {
    const onLinuxBuyClick = () => {
      handler.openBuyWindow()
    }
    return (
      <div className='brave-origin-startup'>
        <div className='container'>
          <img
            src='/assets/Lion.svg'
            className='logo'
            alt=''
          />
          <h1>{getLocale(S.BRAVE_ORIGIN_STARTUP_TITLE)}</h1>
          <div className='description'>
            <p>{getLocale(S.BRAVE_ORIGIN_STARTUP_LINUX_DESCRIPTION)}</p>
            {renderLinuxDescription2(onLinuxBuyClick)}
          </div>
          <div className='buttons'>
            <Button
              kind='outline'
              size='small'
              onClick={onLinuxBuyClick}
            >
              {getLocale(S.BRAVE_ORIGIN_STARTUP_LINUX_BUY_BUTTON)}
            </Button>
            <Button
              kind='plain-faint'
              size='small'
              onClick={() => handler.proceedFree()}
            >
              {getLocale(S.BRAVE_ORIGIN_STARTUP_LINUX_FREE_BUTTON)}
            </Button>
          </div>
        </div>
      </div>
    )
  }

  return (
    <div className='brave-origin-startup'>
      <div className='container'>
        <img
          src='/assets/Lion.svg'
          className='logo'
          alt=''
        />
        <h1>{getLocale(S.BRAVE_ORIGIN_STARTUP_TITLE)}</h1>
        <div className='description'>
          <p>{getLocale(S.BRAVE_ORIGIN_STARTUP_DESCRIPTION)}</p>
          <p>{getLocale(S.BRAVE_ORIGIN_STARTUP_DESCRIPTION2)}</p>
        </div>
        <div className='buttons'>
          <Button
            kind='filled'
            onClick={onRestoreClick}
          >
            {getLocale(S.BRAVE_ORIGIN_STARTUP_RESTORE_BUTTON)}
          </Button>
          <Button
            kind='plain-faint'
            onClick={onBuyClick}
          >
            {getLocale(S.BRAVE_ORIGIN_STARTUP_BUY_BUTTON)}
          </Button>
        </div>
      </div>
    </div>
  )
}
