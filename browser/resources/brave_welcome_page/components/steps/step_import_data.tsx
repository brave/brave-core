/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { StepContentProps, StepFooterProps } from '../types'

const browserProfiles = [
  { name: 'Google Chrome', icon: 'chromerelease-color', profile: 'Agustín Ruiz' },
  { name: 'Google Chrome', icon: 'chromerelease-color', profile: 'Work' },
  { name: 'Microsoft Edge', icon: 'edge-color' },
  { name: 'Vivaldi', icon: 'vivaldi-color' },
  { name: 'DuckDuckGo', icon: 'duckduckgo-color' },
  { name: 'Firefox', icon: 'firefox-color' },
  { name: 'Edge', icon: 'edge-color' },
  { name: 'Ecosia', icon: 'ecosia-color' },
  { name: 'Safari', icon: 'safari-color' }
]

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
        <div className="browser-dropdown">
          <div className="browser-dropdown-header">
            <div className="browser-icons-grid">
              <Icon name="chromerelease-color" />
              <Icon name="safari-color" />
              <Icon name="firefox-color" />
              <Icon name="edge-color" />
            </div>
            <h3>Select your previous browser</h3>
          </div>
          <div className="browser-dropdown-list">
            {browserProfiles.map((browser, index) => (
              <div key={index} className="browser-item">
                <div className="browser-item-icon">
                  <Icon name={browser.icon} />
                </div>
                <span className="browser-item-name">{browser.name}</span>
                {browser.profile && (
                  <Label mode="loud" color="neutral">{browser.profile}</Label>
                )}
              </div>
            ))}
          </div>
        </div>
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

