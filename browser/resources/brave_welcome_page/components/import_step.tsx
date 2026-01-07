/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { BrowserProfile, ImportDataType } from '../api/welcome_api'
import { getString } from '../lib/strings'
import { useStepTransition } from './use_step_transition'
import { useWelcomeApi } from '../api/welcome_api_context'
import {
  getProfileDataTypes,
  splitImportProfileName,
} from '../lib/import_profile_helper'
import { useImportableProfiles } from './use_importable_profiles'
import { StepHeader } from './step_header'
import { ImportProfileSelector } from './import_profile_selector'
import { ImportProfileDetail } from './import_profile_detail'
import { ImportStatusView } from './import_status_view'

import { style } from './import_step.style'

interface Props {
  onBack: () => void
  onNext: () => void
}

export function ImportStep(props: Props) {
  useStepTransition()

  const api = useWelcomeApi()

  const [selectedProfile, setSelectedProfile] =
    React.useState<BrowserProfile | null>(null)
  const [dataTypes, setDataTypes] = React.useState<ImportDataType[]>([])

  const profiles = useImportableProfiles()
  const importStatus = api.useImportDataStatus().data

  const browserProfiles = React.useMemo(() => {
    const counts = new Map<string, number>()
    for (const profile of profiles ?? []) {
      const { browserName } = splitImportProfileName(profile.name)
      if (browserName) {
        counts.set(browserName, (counts.get(browserName) ?? 0) + 1)
      }
    }
    return counts
  }, [profiles])

  React.useEffect(() => {
    api.importDataStatus.update('')
  }, [])

  if (!profiles || profiles.length === 0) {
    return null
  }

  const canImport = selectedProfile && !importStatus && dataTypes.length > 0
  const canNavigate = importStatus !== 'inProgress'
  const canSkip = canNavigate && importStatus !== 'succeeded'

  function onSelectProfile(profile: BrowserProfile | null) {
    setSelectedProfile(profile)
    setDataTypes(getProfileDataTypes(profile))
  }

  function toggleDataType(dataType: ImportDataType, enabled: boolean) {
    const set = new Set(dataTypes)
    if (enabled) {
      set.add(dataType)
    } else {
      set.delete(dataType)
    }
    setDataTypes([...set])
  }

  function startImport() {
    if (selectedProfile && !importStatus) {
      api.importData(selectedProfile.index, new Set(dataTypes))
    }
  }

  function renderUI() {
    if (!selectedProfile) {
      return (
        <ImportProfileSelector
          profiles={profiles ?? []}
          browserProfiles={browserProfiles}
          onSelect={onSelectProfile}
        />
      )
    }
    if (importStatus) {
      return (
        <ImportStatusView
          profile={selectedProfile}
          dataTypes={dataTypes}
          status={importStatus}
        />
      )
    }
    return (
      <ImportProfileDetail
        profile={selectedProfile}
        browserProfiles={browserProfiles}
        onClearSelectedProfile={() => onSelectProfile(null)}
        dataTypes={dataTypes}
        onDataTypeChanged={toggleDataType}
      />
    )
  }

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>{getString('WELCOME_PAGE_IMPORT_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_IMPORT_STEP_TEXT1')}</p>
          <p>{getString('WELCOME_PAGE_IMPORT_STEP_TEXT2')}</p>
        </div>
        <div className='step-ui'>{renderUI()}</div>
      </div>
      <footer>
        <div className='back'>
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onBack}
            isDisabled={!canNavigate}
          >
            {getString('WELCOME_PAGE_BACK_BUTTON_LABEL')}
          </Button>
        </div>
        <div className='forward'>
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onNext}
            isDisabled={!canSkip}
          >
            {getString('WELCOME_PAGE_SKIP_IMPORT_BUTTON_LABEL')}
          </Button>
          {importStatus === 'succeeded' ? (
            <Button
              kind='filled'
              size='large'
              onClick={props.onNext}
              isDisabled={false}
            >
              {getString('WELCOME_PAGE_CONTINUE_BUTTON_LABEL')}
            </Button>
          ) : (
            <Button
              kind='filled'
              size='large'
              onClick={startImport}
              isDisabled={!canImport}
            >
              {getString('WELCOME_PAGE_IMPORT_BUTTON_LABEL')}
            </Button>
          )}
        </div>
      </footer>
    </div>
  )
}
