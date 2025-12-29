/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { StepContentProps, StepFooterProps } from '../types'

export function StepImportDataContent({}: StepContentProps) {
  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Bring along your old browser content</h1>
          <p>Import everything from your old browser and make the transition seamless.</p>
          <p>You can also do this later in Brave's settings.</p>
        </div>
      </div>
      <div className="right-content">
        Hola paso 2
      </div>
    </div>
  )
}

export function StepImportDataFooter({ onNext, onBack, onSkip }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Import
        </Button>
      </div>
    </div>
  )
}

