/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { BrowserProfile } from '../api/welcome_api'
import { getString } from '../lib/strings'
import { BrowserIcon } from './browser_icon'
import { splitImportProfileName } from '../lib/import_profile_helper'

import { style } from './import_profile_selector.style'

interface Props {
  profiles: BrowserProfile[]
  browserProfiles: Map<string, number>
  onSelect: (profile: BrowserProfile) => void
}

export function ImportProfileSelector(props: Props) {
  const { profiles, browserProfiles } = props
  return (
    <div data-css-scope={style.scope}>
      <div className='header'>
        <div className='icon-grid'>
          <Icon name='chromerelease-color' />
          <Icon name='safari-color' />
          <Icon name='firefox-color' />
          <Icon name='edge-color' />
        </div>
        <h3>{getString('WELCOME_PAGE_SELECT_BROWSER_TITLE')}</h3>
      </div>
      <div className='options'>
        {profiles.map((profile) => {
          const { browserName, profileName } = splitImportProfileName(
            profile.name,
          )
          const showProfileName =
            browserName
            && profileName
            && (browserProfiles.get(browserName) ?? 0) > 1
          return (
            <button
              key={profile.index}
              onClick={() => props.onSelect(profile)}
            >
              <div>
                <BrowserIcon name={browserName} />
              </div>
              <h4>{browserName ?? profileName}</h4>
              {showProfileName && (
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
