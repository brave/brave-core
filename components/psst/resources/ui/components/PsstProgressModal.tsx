/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ModalTitle } from './basic/display'
import {
  Container,
  HorizontalContainer,
  LeftAlignedItem,
  PsstDlgButton,
  RightAlignedItem,
  TextSection,
} from './basic/structure'
import SettingsCard from './SettingsCard'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { PsstStrings } from 'gen/components/grit/brave_components_webui_strings'
import { getLocale } from '$web-common/locale'
import { usePsstDialogAPI } from '../api/psst_dialog_api_context'
import { SettingCardDataItem } from '../api/psst_dialog_api'

export interface Props {}

export enum SettingState {
  None,
  Selection,
  Progress,
  Completed,
  Failed,
}

interface OptionStatus {
  url: string
  description: string
  error: string | null
  checked: boolean
  disabled: boolean
  settingState: SettingState
}

export interface PsstProgressModalState {
  commonState: SettingState
  site_name: string
  optionsStatuses: Map<string, OptionStatus> | null
}

export default function PsstProgressModal(_props: Props) {
  
  const { api: browserProxy } = usePsstDialogAPI()
  
  const [commonState, setCommonState] = React.useState<SettingState>(SettingState.None)
  const [siteName, setSiteName] = React.useState<string>('')
  const [optionsStatuses, setOptionsStatuses] = React.useState<Map<string, OptionStatus> | null>(new Map())

  // Subscribe to API data at top level - with null checks
  const { data: settingsData } = browserProxy?.useSettingsCardData?.() || { data: null }
  const { data: requestStatus } = browserProxy?.useRequestStatus?.() || { data: null }
  const { data: completionStatus } = browserProxy?.useCompletionStatus?.() || { data: null }
  const { mutate: applyChanges } = browserProxy?.useApplyChanges?.() || { mutate: () => {} }

  const setStateProp = React.useCallback((
    updates: Partial<OptionStatus>,
    predicate: (status: OptionStatus) => boolean = () => true,
  ) => {
    setOptionsStatuses((prevOptionsStatuses) => {
      if (!prevOptionsStatuses) {
        return prevOptionsStatuses
      }
      const updatedOptionsStatuses = new Map<string, OptionStatus>()
      prevOptionsStatuses.forEach((status, key) => {
        if (predicate(status)) {
          updatedOptionsStatuses.set(key, { ...status, ...updates })
        } else {
          updatedOptionsStatuses.set(key, status)
        }
      })
      return updatedOptionsStatuses
    })
  }, [])

  const setPropForUrl = React.useCallback((targetUrl: string, updates: Partial<OptionStatus>) => {
    setOptionsStatuses((prevOptionsStatuses) => {
      if (!prevOptionsStatuses) {
        return prevOptionsStatuses
      }

      const updatedOptionsStatuses = new Map<string, OptionStatus>()
      let found = false
      prevOptionsStatuses.forEach((status, key) => {
        if (status.url === targetUrl) {
          updatedOptionsStatuses.set(key, { ...status, ...updates })
          found = true
        } else {
          updatedOptionsStatuses.set(key, status)
        }
      })
      if (found) {
        return updatedOptionsStatuses
      }
      return prevOptionsStatuses
    })
  }, [])

  // Handle settings data updates
  React.useEffect(() => {
    if (settingsData) {
      const checkedUrlsMap = new Map<string, OptionStatus>()
      settingsData.items.forEach((item: SettingCardDataItem) => {
        checkedUrlsMap.set(item.url, {
          url: item.url,
          description: item.description,
          error: null,
          checked: true,
          disabled: false,
          settingState: SettingState.Selection,
        })
      })
      setSiteName(settingsData.siteName)
      setOptionsStatuses(checkedUrlsMap)
    }
  }, [settingsData])

  // Handle request status updates
  React.useEffect(() => {
    if (requestStatus) {
      setPropForUrl(requestStatus.url, {
        settingState: requestStatus.error ? SettingState.Failed : SettingState.Completed,
        error: requestStatus.error || null,
      })
    }
  }, [requestStatus, setPropForUrl])

  // Handle completion status updates
  React.useEffect(() => {
    if (completionStatus && (completionStatus.appliedChecks || completionStatus.errors)) {
      setCommonState(SettingState.Completed)
    }
  }, [completionStatus])

  const handleSettingItemCheck = React.useCallback((url: string, checked: boolean) => {
    setOptionsStatuses((prevOptionsStatuses) => {
      if (!prevOptionsStatuses) return prevOptionsStatuses
      
      const os = prevOptionsStatuses.get(url)
      if (os) {
        const newMap = new Map(prevOptionsStatuses)
        newMap.set(url, {
          checked: checked,
          url: os.url,
          description: os.description,
          error: os.error,
          disabled: os.disabled,
          settingState: os.settingState,
        })
        return newMap
      }
      return prevOptionsStatuses
    })
  }, [])

  const closeDialog = React.useCallback(() => {
    browserProxy.closeDialog()
  }, [browserProxy])

  const handleApplyChanges = React.useCallback(() => {
    let settingsToProcess: string[] = []
    if (optionsStatuses) {
      settingsToProcess = Array.from(optionsStatuses.entries())
        .filter(([_, value]) => !value.checked)
        .map(([key]) => key)
    }
    
    setStateProp(
      { settingState: SettingState.Progress },
      (status) => status.checked,
    )
    setStateProp(
      { disabled: true },
      (status) => !status.checked,
    )
    setCommonState(SettingState.Progress)
    
    applyChanges([siteName, settingsToProcess])
  }, [optionsStatuses, siteName, setStateProp, applyChanges])

  const isInProgress = commonState === SettingState.Progress

  return (
    <Container>
      <HorizontalContainer>
        <LeftAlignedItem>
          <TextSection>
            <ModalTitle>
              {getLocale(PsstStrings.PSST_CONSENT_DIALOG_TITLE)}
            </ModalTitle>
          </TextSection>
        </LeftAlignedItem>
        <RightAlignedItem>
          <Button
            fab
            kind='plain-faint'
            isDisabled={isInProgress}
            onClick={closeDialog}
          >
            <Icon name={'close-circle'} />
          </Button>
        </RightAlignedItem>
      </HorizontalContainer>
      <TextSection>
        {getLocale(PsstStrings.PSST_CONSENT_DIALOG_TITLE)}
      </TextSection>
      <SettingsCard
        title={getLocale(PsstStrings.PSST_CONSENT_DIALOG_OPTIONS_TITLE)}
        subTitle={siteName}
        progressModelState={{ commonState, site_name: siteName, optionsStatuses }}
        onItemChecked={handleSettingItemCheck}
      />
      {(() => {
        if (commonState !== SettingState.Completed) {
          return (
            <RightAlignedItem>
              <PsstDlgButton
                kind='outline'
                size='medium'
                isDisabled={isInProgress}
                onClick={closeDialog}
              >
                {getLocale(PsstStrings.PSST_COMPLETE_CONSENT_DIALOG_CANCEL)}
              </PsstDlgButton>
              <PsstDlgButton
                id='psst-dialog-ok-btn'
                kind='filled'
                size='medium'
                isDisabled={isInProgress}
                isLoading={isInProgress}
                onClick={handleApplyChanges}
              >
                {getLocale(PsstStrings.PSST_COMPLETE_CONSENT_DIALOG_OK)}
              </PsstDlgButton>
            </RightAlignedItem>
          )
        } else {
          return (
            <RightAlignedItem>
              <PsstDlgButton
                kind='outline'
                size='medium'
                isDisabled={false}
                onClick={closeDialog}
              >
                {getLocale(
                  PsstStrings.PSST_COMPLETE_CONSENT_DIALOG_REPORT_FAILED,
                )}
              </PsstDlgButton>
              <PsstDlgButton
                kind='filled'
                size='medium'
                isDisabled={false}
                isLoading={false}
                onClick={closeDialog}
              >
                {getLocale(PsstStrings.PSST_COMPLETE_CONSENT_DIALOG_CLOSE)}
              </PsstDlgButton>
            </RightAlignedItem>
          )
        }
      })()}
    </Container>
  )
}
