/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { StepContentProps, StepFooterProps } from '../types'

export function StepCompleteContent({}: StepContentProps) {
  return (
    <div className="content">
      <div className="left-content">
        <div className="brave-logo-container">
          <Icon name='social-brave-release-favicon-fullheight-color'/>
        </div>
        <div className="left-text-content">
          <h1>You're all set!</h1>
          <p>Brave is ready. Start browsing with privacy, speed, and rewards.</p>
        </div>
      </div>
      <div className="right-content">
        {/* Right content for completion step - success animation, tips, etc. */}
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

