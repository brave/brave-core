/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { StepContentProps, StepFooterProps } from '../types'
import welcomeImage from '../img/welcome.png'

export function StepWelcomeContent({}: StepContentProps) {
  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Get ready for the best browsing experience</h1>
          <p>Experience the web without being watched. Brave shields you from trackers, ads, and fingerprinting, all automatically.</p>
        </div>
      </div>
      <div className="right-content">
        <img src={welcomeImage} alt="Welcome to Brave" className="hero-image" />
      </div>
    </div>
  )
}

export function StepWelcomeFooter({ onNext, onBack, onSkip, isFirstStep }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        {!isFirstStep && (
          <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
        )}
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Set Brave as default
        </Button>
      </div>
    </div>
  )
}

