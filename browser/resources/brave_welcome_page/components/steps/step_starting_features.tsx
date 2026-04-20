/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'

import { StepContentProps, StepFooterProps } from '../types'

import aiIcon from '../img/ai.svg'
import web3Icon from '../img/web3.svg'
// TODO: Replace these placeholder imports with actual SVG icons
// import anonymousDataIcon from '../img/anonymous-data.svg'
// import defaultBrowserIcon from '../img/default-browser.svg'

interface FeatureOption {
  id: string
  title: string
  subtitle: string
  icon: string | null // SVG import path
}

const featureOptions: FeatureOption[] = [
  {
    id: 'ai',
    title: 'Brave AI',
    subtitle: 'The smart AI assistant built right into your browser. Ask questions, summarize pages, create new content, and more. Privately.',
    icon: aiIcon
  },
  {
    id: 'web3',
    title: 'Web3',
    subtitle: 'The secure, built-in crypto wallet that supercharges your browser for Web3.',
    icon: web3Icon
  }
]

export function StepStartingFeaturesContent({}: StepContentProps) {
  const [checkedOptions, setCheckedOptions] = React.useState<Record<string, boolean>>(
    Object.fromEntries(featureOptions.map(opt => [opt.id, true]))
  )

  const handleCardClick = (id: string) => {
    setCheckedOptions(prev => ({ ...prev, [id]: !prev[id] }))
  }

  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
        <h1>Make Brave yours</h1>
        <p>Select the features you want enabled by default in the browser. You can always enable or disable these later in Settings.</p>
        </div>
      </div>
      <div className="right-content">
        <div className="privacy-cards">
          {featureOptions.map((option) => (
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

export function StepStartingFeaturesFooter({ onNext, onBack }: StepFooterProps) {
  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="filled" size="large" className='main-button' onClick={onNext}>
          Continue
        </Button>
      </div>
    </div>
  )
}
