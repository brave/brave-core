/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { StepContentProps, StepFooterProps } from '../types'
import wdpImage from '../img/wdp.png'

export function StepBraveSearchContent({}: StepContentProps) {
  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Help Brave Search grow</h1>
          <p>Opt into the Web Discovery Project to help grow the index and improve search results.</p>
        </div>
      </div>
      <div className="right-content">
        <img src={wdpImage} alt="Help Brave Search grow" className="hero-image" />
      </div>
    </div>
  )
}

export function StepBraveSearchFooter({ onNext, onBack, onSkip }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Sure, I'll help
        </Button>
      </div>
    </div>
  )
}

