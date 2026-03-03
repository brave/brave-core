/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import styles from './styles'

type View = 'main' | 'restore'

function BraveOriginStartupApp() {
  const [currentView, setCurrentView] = React.useState<View>('main')
  const [purchaseId, setPurchaseId] = React.useState('')
  const [verifying, setVerifying] = React.useState(false)
  const [error, setError] = React.useState('')

  const onRestoreClick = () => {
    setCurrentView('restore')
    setError('')
    setPurchaseId('')
  }

  const onBuyClick = () => {
    console.log('[Storybook] openBuyWindow called')
    setCurrentView('restore')
    setError('')
    setPurchaseId('')
  }

  const onVerifyClick = async () => {
    if (!purchaseId || verifying) return
    setVerifying(true)
    setError('')
    // Simulate verification delay
    await new Promise((r) => setTimeout(r, 1500))
    setVerifying(false)
    if (purchaseId === 'valid') {
      console.log('[Storybook] closeDialog called — purchase verified')
    } else {
      setError('Invalid purchase ID. Please try again.')
    }
  }

  if (currentView === 'restore') {
    return (
      <div style={styles.host}>
        <div style={styles.container}>
          <Icon
            name='social-brave-release-favicon-fullheight-color'
            style={styles.logo}
          />
          <h1 style={styles.h1}>Restore Purchase</h1>
          <div style={styles.description}>
            <p style={styles.descriptionP}>
              Enter your purchase ID to activate your Brave Origin purchase.
            </p>
          </div>
          <div style={styles.restoreContainer}>
            <div style={styles.inputGroup}>
              <label style={styles.label}>Purchase ID</label>
              <input
                type='text'
                style={styles.input}
                value={purchaseId}
                onChange={(e) => {
                  setPurchaseId(e.target.value.trim())
                  setError('')
                }}
                placeholder='Enter your purchase ID'
                disabled={verifying}
              />
              {error && <div style={styles.errorMessage}>{error}</div>}
              {verifying && (
                <div style={styles.verifyingMessage}>
                  <ProgressRing
                    style={{ '--leo-progressring-size': '16px' } as any}
                  />
                  Verifying...
                </div>
              )}
            </div>
            <div style={styles.buttons}>
              <button
                style={{
                  ...styles.btnPrimary,
                  ...(verifying || !purchaseId
                    ? styles.btnPrimaryDisabled
                    : {}),
                }}
                onClick={onVerifyClick}
                disabled={verifying || !purchaseId}
              >
                {verifying ? 'Verifying...' : 'Verify purchase ID'}
              </button>
              <button
                style={styles.btnSecondary}
                onClick={onBuyClick}
              >
                Buy Brave Origin
              </button>
            </div>
          </div>
        </div>
      </div>
    )
  }

  return (
    <div style={styles.host}>
      <div style={styles.container}>
        <Icon
          name='social-brave-release-favicon-fullheight-color'
          style={styles.logo}
        />
        <h1 style={styles.h1}>Brave Origin</h1>
        <div style={styles.description}>
          <p style={styles.descriptionP}>
            Welcome to Brave Origin. To continue, please buy or restore your
            Brave Origin purchase.
          </p>
          <p style={{ ...styles.descriptionP, marginBottom: 0 }}>
            You need an active Brave Origin purchase to use this browser.
          </p>
        </div>
        <div style={styles.buttons}>
          <button
            style={styles.btnPrimary}
            onClick={onRestoreClick}
          >
            Restore Brave Origin purchase
          </button>
          <button
            style={styles.btnSecondary}
            onClick={onBuyClick}
          >
            Buy Brave Origin
          </button>
        </div>
      </div>
    </div>
  )
}

export default {
  title: 'Brave Origin/Startup Dialog',
  component: BraveOriginStartupApp,
  decorators: [
    (Story: any) => (
      <div style={{ width: '500px', height: '500px', margin: '0 auto' }}>
        <Story />
      </div>
    ),
  ],
} satisfies Meta<typeof BraveOriginStartupApp>

export const MainView: StoryObj<typeof BraveOriginStartupApp> = {}
