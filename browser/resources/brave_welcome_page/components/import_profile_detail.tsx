/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { BrowserProfile, ImportDataType } from '../api/welcome_api'
import { getString } from '../lib/strings'
import { BrowserIcon } from './browser_icon'
import { ImportDataTypeName } from './import_data_type_name'

import {
  splitImportProfileName,
  getProfileDataTypes,
} from '../lib/import_profile_helper'

import { style } from './import_profile_detail.style'

const dataTypePosition: Record<ImportDataType, number> = {
  'favorites': 1,
  'history': 2,
  'passwords': 3,
  'extensions': 4,
  'autofillFormData': 5,
  'search': 6,
}

interface Props {
  profile: BrowserProfile
  browserProfiles: Map<string, number>
  onClearSelectedProfile: () => void
  dataTypes: ImportDataType[]
  onDataTypeChanged: (type: ImportDataType, checked: boolean) => void
}

export function ImportProfileDetail(props: Props) {
  const { profile, browserProfiles, dataTypes } = props
  const { browserName, profileName } = splitImportProfileName(profile.name)

  const availableTypes = React.useMemo(() => {
    return getProfileDataTypes(profile).sort((a, b) => {
      return dataTypePosition[a] - dataTypePosition[b]
    })
  }, [profile])

  const showProfileName =
    browserName && profileName && (browserProfiles.get(browserName) ?? 0) > 1

  return (
    <div data-css-scope={style.scope}>
      <button
        className='selected'
        onClick={props.onClearSelectedProfile}
      >
        <div>
          <BrowserIcon name={browserName} />
        </div>
        <h3>{browserName ?? profileName}</h3>
        {showProfileName && (
          <Label
            mode='loud'
            color='neutral'
          >
            {profileName}
          </Label>
        )}
        <Icon
          className='carat'
          name='carat-down'
        />
      </button>
      <div className='data-types'>
        <h4>{getString('WELCOME_PAGE_IMPORT_DATA_TYPES_TITLE')}</h4>
        <div className='list'>
          {availableTypes.map((type) => (
            <div key={type}>
              <Checkbox
                checked={dataTypes.includes(type)}
                onChange={(event) => {
                  props.onDataTypeChanged(type, event.checked)
                }}
              >
                <ImportDataTypeName dataType={type} />
              </Checkbox>
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}
