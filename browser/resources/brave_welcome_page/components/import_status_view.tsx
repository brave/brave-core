/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import {
  BrowserProfile,
  ImportDataType,
  ImportDataStatus,
} from '../api/welcome_api'

import { BrowserIcon } from './browser_icon'
import { ImportDataTypeName } from './import_data_type_name'
import { splitImportProfileName } from '../lib/import_profile_helper'

import { style } from './import_status_view.style'

interface Props {
  profile: BrowserProfile
  dataTypes: ImportDataType[]
  status: ImportDataStatus
}

export function ImportStatusView(props: Props) {
  const { profile, dataTypes, status } = props
  const { browserName } = splitImportProfileName(profile.name)

  function headerText() {
    switch (status) {
      case '':
        return ''
      case 'failed':
        return ''
      case 'succeeded':
        return 'Import results'
      case 'inProgress':
        return 'Importing your content'
    }
  }

  function progressIndicator() {
    if (status === 'inProgress') {
      return <ProgressRing />
    }
    return (
      <Icon
        name='check-circle-filled'
        className='success-icon'
      />
    )
  }

  return (
    <div
      data-css-scope={style.scope}
      data-import-status={status}
    >
      <div className='header'>
        <BrowserIcon name={browserName} />
        <div className='carats'>
          <Icon name='carat-right' />
          <Icon name='carat-right' />
          <Icon name='carat-right' />
        </div>
        <div className='brave-icon'>
          <BrowserIcon name='Brave' />
          {progressIndicator()}
        </div>
      </div>
      <div className='data-types'>
        <h4>{headerText()}</h4>
        <div className='list'>
          {dataTypes.map((type) => (
            <div key={type}>
              {progressIndicator()}
              <div className='data-type-name'>
                <ImportDataTypeName dataType={type} />
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}
