/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { StepContentProps, StepFooterProps } from '../types'

export function StepCompleteContent({}: StepContentProps) {
  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Build a better web with us</h1>
          <p>Help Brave be better and create better products by enabling these private metrics. We promise to protect your privacy in everything we do.</p>
          <p>You can change these choices at any time in Brave at brave://settings/privacy. Read our full Privacy Policy.</p>
        </div>
      </div>
      <div className="right-content">
      Content from step 5
      </div>
    </div>
  )
}

export function StepCompleteFooter({ onNext, onBack }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Start browsing
        </Button>
      </div>
    </div>
  )
}

