/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import SettingsCard from './SettingsCard'
import { Container, PsstDlgButton, RightAlignedItem } from './basic/structure'
import { usePsstDialogAPI } from '../api/psst_dialog_api_context'

import '../strings'

// Styled components
const ModalTitle = styled.div`
  margin: ${leo.spacing['2Xl']} 0 ${leo.spacing['2Xl']} 0;
  gap: ${leo.spacing.xl};
  font: ${font.heading.h4};
  font-size: ${leo.typography.heading.h4.fontSize};
  line-height: ${leo.typography.heading.h4.lineHeight};
  color: ${color.text.secondary};
`
const ModalTitleBody = styled.div`
  font: ${leo.font.default.regular};
  color: ${leo.color.text.secondary};
  letter-spacing: ${leo.typography.letterSpacing.small};
  margin-bottom: ${leo.spacing['2Xl']};
`
export enum SettingState {
  None,
  Selection,
  Progress,
  Completed,
  Failed,
}

export interface OptionStatus {
  uid: string
  description: string
  error: string | null
  checked: boolean
  disabled: boolean
  settingState: SettingState
}

export const PsstProgressModal = () => {
  const { api, siteData } = usePsstDialogAPI()

  const [optionsStatuses, updateAllMatchingOptionsStatuses] =
    React.useState<OptionStatus[]>()

  const commonState: SettingState = (() => {
    if (!optionsStatuses) return SettingState.None

    const allDone = optionsStatuses.every(
      (option) =>
        option.settingState === SettingState.Failed
        || option.settingState === SettingState.Completed,
    )

    if (!allDone) {
      const hasProgress = optionsStatuses.some(
        (option) => option.settingState === SettingState.Progress,
      )
      return hasProgress ? SettingState.Progress : SettingState.None
    }

    const hasFailures = optionsStatuses.some(
      (option) => option.settingState === SettingState.Failed,
    )
    return hasFailures ? SettingState.Failed : SettingState.Completed
  })()

  const { performPrivacyTuning } = api.usePerformPrivacyTuning()

  const siteName = siteData ? siteData.siteName : ''

  React.useEffect(() => {
    if (!siteData) return

    const optionStatusArray: OptionStatus[] = siteData.items.map((item) => ({
      uid: item.uid,
      description: item.description,
      error: null,
      checked: true,
      disabled: false,
      settingState: SettingState.Selection,
    }))
    updateAllMatchingOptionsStatuses(optionStatusArray)
  }, [siteData])

  // Handle request status updates
  api.useOnSetRequestStatus((requestUid, requestError) => {
    if (!requestUid) return

    updateAllMatchingOptionsStatuses((prevOptionsStatuses) => {
      if (!prevOptionsStatuses) return prevOptionsStatuses

      const index = prevOptionsStatuses.findIndex(
        (status) => status.uid === requestUid,
      )
      if (index === -1) return prevOptionsStatuses

      const updatedOptions = [...prevOptionsStatuses]
      updatedOptions[index] = {
        ...updatedOptions[index],
        settingState: requestError
          ? SettingState.Failed
          : SettingState.Completed,
        error: requestError || null,
      }

      return updatedOptions
    })
  })

  const handleSettingItemCheck = React.useCallback(
    (uid: string, checked: boolean) => {
      updateAllMatchingOptionsStatuses((prevOptionsStatuses) => {
        if (!prevOptionsStatuses) return prevOptionsStatuses

        const index = prevOptionsStatuses.findIndex(
          (status) => status.uid === uid,
        )
        if (index === -1) return prevOptionsStatuses

        const updatedOptions = [...prevOptionsStatuses]
        updatedOptions[index] = {
          ...updatedOptions[index],
          checked,
        }

        return updatedOptions
      })
    },
    [],
  )

  const handleApplyChanges = React.useCallback(() => {
    let enabledUids: string[] = []
    if (optionsStatuses) {
      enabledUids = optionsStatuses
        .filter((option) => option.checked)
        .map((option) => option.uid)
      const newOptionsStatuses: OptionStatus[] = optionsStatuses.map(
        (option) => {
          if (option.checked) {
            return {
              ...option,
              settingState: SettingState.Progress,
            }
          } else {
            return {
              ...option,
              disabled: true,
            }
          }
        },
      )
      updateAllMatchingOptionsStatuses(newOptionsStatuses)
    }

    performPrivacyTuning([enabledUids])
  }, [optionsStatuses, performPrivacyTuning])

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
            onClick={api.closeDialog}
          >
            <Icon name={'close-circle'} />
          </Button>
        </RightAlignedItem>
      </Flex>
      <ModalTitleBody>{getLocale(S.PSST_CONSENT_DIALOG_BODY)}</ModalTitleBody>
      <SettingsCard
        title={siteName}
        progressModelState={{
          siteName,
          optionsStatuses,
        }}
        onItemChecked={handleSettingItemCheck}
      />
      {commonState !== SettingState.Failed ? (
        <RightAlignedItem>
          <PsstDlgButton
            kind='outline'
            size='medium'
            isDisabled={isInProgress}
            onClick={api.closeDialog}
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
            isDisabled={isInProgress}
            isLoading={isInProgress}
            onClick={api.reportFailedContent}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_REPORT_FAILED)}
          </PsstDlgButton>
          <PsstDlgButton
            kind='filled'
            size='medium'
            isDisabled={isInProgress}
            isLoading={isInProgress}
            onClick={api.closeDialog}
          >
            {getLocale(S.PSST_COMPLETE_CONSENT_DIALOG_CLOSE)}
          </PsstDlgButton>
        </RightAlignedItem>
      )}
    </Container>
  )
}
