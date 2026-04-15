/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'
import { Container, PsstDlgButton, RightAlignedItem } from './basic/structure'
import SettingsCard from './SettingsCard'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import { usePsstDialogAPI } from '../api/psst_dialog_api_context'
import { SettingCardDataItem } from '../api/psst_dialog_api'
import '../strings'

// Styled components
const ModalTitle = styled.div`
  font: ${font.heading.h4};
  color: ${color.text.secondary};
`
const ModalTitleBody = styled.div`
  font: ${font.default.regular};
  color: ${color.text.primary};
`
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

export const PsstProgressModal = () => {
  const psstDialogContext = usePsstDialogAPI()
  const { api } = psstDialogContext

  const [commonState, setCommonState] = React.useState<SettingState>(
    SettingState.None,
  )
  const [siteName, setSiteName] = React.useState<string>('')
  const [optionsStatuses, setOptionsStatuses] = React.useState<Map<
    string,
    OptionStatus
  > | null>(new Map())

  // Subscribe to API state endpoints (data is pushed via callback router)
  // Add defensive checks for api
  const settingsData = api?.useCurrentSetSettingsCardData?.()
  const requestStatus = api?.useCurrentOnSetRequestDone?.()
  const completionStatus = api?.useCurrentOnSetCompleted?.()
  const { applyChanges } = api?.useApplyChanges?.() || {
    applyChanges: () => {},
  }

  // Extract specific values to avoid object reference issues in useEffect dependencies
  const settingCardData = settingsData?.data?.[0]
  const [appliedChecks, completionErrors] = completionStatus?.data || []

  const closeDialog = React.useCallback(() => {
    api.closeDialog()
  }, [api])

  const setStateProp = React.useCallback(
    (
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
    },
    [],
  )

  const setPropForUrl = React.useCallback(
    (targetUrl: string, updates: Partial<OptionStatus>) => {
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
    },
    [],
  )

  // Handle settings data updates
  React.useEffect(() => {
    if (settingCardData) {
      const checkedUrlsMap = new Map<string, OptionStatus>()
      settingCardData.items.forEach((item: SettingCardDataItem) => {
        checkedUrlsMap.set(item.url, {
          url: item.url,
          description: item.description,
          error: null,
          checked: true,
          disabled: false,
          settingState: SettingState.Selection,
        })
      })
      setSiteName(settingCardData.siteName || '')
      setOptionsStatuses(checkedUrlsMap)
    }
  }, [settingCardData])

  // Handle request status updates
  React.useEffect(() => {
    if (requestStatus?.data) {
      const [requestUrl, requestError] = requestStatus.data
      if (requestUrl) {
        setPropForUrl(requestUrl, {
          settingState: requestError
            ? SettingState.Failed
            : SettingState.Completed,
          error: requestError || null,
        })
      }
    }
  }, [requestStatus, setPropForUrl])

  // Handle completion status updates
  React.useEffect(() => {
    if (completionErrors) {
      setCommonState(SettingState.Failed)
      return
    }

    if (appliedChecks) {
      setCommonState(SettingState.Completed)
    }
  }, [appliedChecks, completionErrors])

  const handleSettingItemCheck = React.useCallback(
    (url: string, checked: boolean) => {
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
    },
    [],
  )

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
    setStateProp({ disabled: true }, (status) => !status.checked)
    setCommonState(SettingState.Progress)

    applyChanges([siteName, settingsToProcess])
  }, [optionsStatuses, siteName, setStateProp, applyChanges])

  const isInProgress = commonState === SettingState.Progress

  return (
    <Container>
      <Flex
        direction='row'
        justify='space-between'
        align='center'
      >
        <ModalTitle>{getLocale(S.PSST_CONSENT_DIALOG_TITLE)}</ModalTitle>
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
      </Flex>
      <ModalTitleBody>{getLocale(S.PSST_CONSENT_DIALOG_BODY)}</ModalTitleBody>
      <SettingsCard
        title={siteName}
        progressModelState={{
          commonState,
          site_name: siteName,
          optionsStatuses,
        }}
        onItemChecked={handleSettingItemCheck}
      />
      {commonState !== SettingState.Completed ? (
        <RightAlignedItem>
          <PsstDlgButton
            kind='outline'
            size='medium'
            isDisabled={isInProgress}
            onClick={closeDialog}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_CANCEL)}
          </PsstDlgButton>
          <PsstDlgButton
            id='psst-dialog-ok-btn'
            kind='filled'
            size='medium'
            isDisabled={isInProgress}
            isLoading={isInProgress}
            onClick={handleApplyChanges}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_OK)}
          </PsstDlgButton>
        </RightAlignedItem>
      ) : (
        <RightAlignedItem>
          <PsstDlgButton
            kind='outline'
            size='medium'
            onClick={closeDialog}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_REPORT_FAILED)}
          </PsstDlgButton>
          <PsstDlgButton
            kind='filled'
            size='medium'
            onClick={closeDialog}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_CLOSE)}
          </PsstDlgButton>
        </RightAlignedItem>
      )}
    </Container>
  )
}
