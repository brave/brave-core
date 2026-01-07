/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { BrowserProfile } from '../api/welcome_api'
import { BrowserIcon } from './browser_icon'
import { splitImportProfileName } from '../lib/import_profile_helper'

import { style } from './import_profile_selector.style'

interface Props {
  profiles: BrowserProfile[]
  onSelect: (profile: BrowserProfile) => void
}

export function ImportProfileSelector(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <div className='header'>
        <div className='icon-grid'>
          <Icon name='chromerelease-color' />
          <Icon name='safari-color' />
          <Icon name='firefox-color' />
          <Icon name='edge-color' />
        </div>
        <h3>Select your previous browser</h3>
      </div>
      <div className='options'>
        {props.profiles.map((profile) => {
          const { browserName, profileName } = splitImportProfileName(
            profile.name,
          )
          return (
            <button
              key={profile.index}
              onClick={() => props.onSelect(profile)}
            >
              <div>
                <BrowserIcon name={browserName} />
              </div>
              <h4>{browserName ?? profileName}</h4>
              {browserName && profileName && (
                <Label
                  mode='loud'
                  color='neutral'
                >
                  {profileName}
                </Label>
              )}
            </button>
          )
        })}
      </div>
    </div>
  )
}
