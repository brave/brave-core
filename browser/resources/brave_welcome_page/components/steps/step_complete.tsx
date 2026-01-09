/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'

import { StepContentProps, StepFooterProps } from '../types'

import wdpIcon from '../img/wdp.svg'
import setDefaultIcon from '../img/set-default.svg'
import p3aIcon from '../img/p3a.svg'
// TODO: Replace these placeholder imports with actual SVG icons
// import anonymousDataIcon from '../img/anonymous-data.svg'
// import defaultBrowserIcon from '../img/default-browser.svg'

interface PrivacyOption {
  id: string
  title: string
  subtitle: string
  icon: string | null // SVG import path
}

const privacyOptions: PrivacyOption[] = [
  {
    id: 'wdp',
    title: 'Help Brave Search grow',
    subtitle: 'Opt into the Web Discovery Project to help grow the index and improve search results.',
    icon: wdpIcon
  },
  {
    id: 'anonymous-data',
    title: 'Help us make the browser better',
    subtitle: 'Share completely private & anonymous product insights.',
    icon: p3aIcon
  },
  {
    id: 'default-browser',
    title: 'Set Brave as your default browser',
    subtitle: 'Enjoy ad-blocking and real privacy with every page view. Give it a try!',
    icon: setDefaultIcon
  }
]

export function StepCompleteContent({}: StepContentProps) {
  const [checkedOptions, setCheckedOptions] = React.useState<Record<string, boolean>>(
    Object.fromEntries(privacyOptions.map(opt => [opt.id, true]))
  )

  const handleCardClick = (id: string) => {
    setCheckedOptions(prev => ({ ...prev, [id]: !prev[id] }))
  }

  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Build a better web with us</h1>
          <p>Help Brave be better and create better products by enabling these private metrics. We promise to protect your privacy in everything we do.</p>
          <p>You can change these choices at any time in Brave at <a href="#" target="_blank">brave://settings/privacy.</a> Read our full <a href="#" target="_blank">Privacy Policy.</a></p>
        </div>
      </div>
      <div className="right-content">
        <div className="privacy-cards">
          {privacyOptions.map((option) => (
            <div
              key={option.id}
              className={`privacy-card ${!checkedOptions[option.id] ? 'privacy-card-unchecked' : ''}`}
              onClick={() => handleCardClick(option.id)}
            >
              <div className="privacy-card-icon">
                {option.icon && <img src={option.icon} alt="" />}
              </div>
              <div className="privacy-card-content">
                <h3 className="privacy-card-title">{option.title}</h3>
                <p className="privacy-card-subtitle">{option.subtitle}</p>
              </div>
              <Checkbox
                checked={checkedOptions[option.id]}
                onChange={(detail) => setCheckedOptions(prev => ({ ...prev, [option.id]: detail.checked }))}
              />
            </div>
          ))}
        </div>
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

