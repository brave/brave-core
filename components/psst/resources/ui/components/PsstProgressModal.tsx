/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import SettingsCard from './SettingsCard'
import { Container, PsstDlgButton, RightAlignedItem } from './basic/structure'
import { usePsstDialogAPI } from '../api/psst_dialog_api_context'

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
  uid: string
  description: string
  error: string | null
  checked: boolean
  disabled: boolean
  settingState: SettingState
}

export const PsstProgressModal = () => {
  const psstDialogContext = usePsstDialogAPI()
  const { api } = psstDialogContext

  const [commonState, setCommonState] = React.useState<SettingState>(
    SettingState.None,
  )
  const [optionsStatuses, setOptionsStatuses] = React.useState<OptionStatus[]>()

  // Subscribe to API state endpoints (data is pushed via events)
  // Add defensive checks for api
  const settingsData = api.useCurrentSetSettingsCardData()
  const requestStatus = api.useCurrentOnSetRequestDone()
  const completionStatus = api.useCurrentOnSetCompleted()
  const { applyChanges } = api.useApplyChanges()

  // Extract specific values to avoid object reference issues in useEffect dependencies
  const settingCardData = settingsData.data?.[0]
  const siteName = settingCardData?.siteName || ''
  const [appliedChecks, completionErrors] = completionStatus?.data || []
  const [requestUid, requestError] = requestStatus?.data || []


  // Handle settings data updates
  React.useEffect(() => {
    if (settingCardData) {
      const optionStatusArray: OptionStatus[] = settingCardData.items.map(item => ({
        uid: item.uid,
        description: item.description,
        error: null,
        checked: true,
        disabled: false,
        settingState: SettingState.Selection
      }));
      setOptionsStatuses(optionStatusArray)
    }
  }, [settingCardData])

  // Handle request status updates
  React.useEffect(() => {
    if (!requestUid) return;

    setOptionsStatuses(prevOptionsStatuses => {
      if (!prevOptionsStatuses) return prevOptionsStatuses;

      const index = prevOptionsStatuses.findIndex(status => status.uid === requestUid);
      if (index === -1) return prevOptionsStatuses;

      const updatedOptions = [...prevOptionsStatuses];
      updatedOptions[index] = {
        ...updatedOptions[index],
        settingState: requestError ? SettingState.Failed : SettingState.Completed,
        error: requestError || null,
      };

      return updatedOptions;
    });
  }, [requestUid, requestError])

  // Handle completion status updates
  React.useEffect(() => {
    if (completionErrors) {
      setCommonState(SettingState.Failed)
    }

    if (appliedChecks) {
      setCommonState(SettingState.Completed)
    }
  }, [appliedChecks, completionErrors])

  const handleSettingItemCheck = React.useCallback(
    (uid: string, checked: boolean) => {
      setOptionsStatuses((prevOptionsStatuses) => {
        if (!prevOptionsStatuses) return prevOptionsStatuses;

       const index = prevOptionsStatuses.findIndex(status => status.uid === uid);
       if (index === -1) return prevOptionsStatuses;

       const updatedOptions = [...prevOptionsStatuses];
      updatedOptions[index] = {
        ...updatedOptions[index],
        checked,
      };

      return updatedOptions;
      })
    },
    [],
  )

  const handleApplyChanges = React.useCallback(() => {
    let disabledUids: string[] = []
    if (optionsStatuses) {
      disabledUids = optionsStatuses
        .filter(option => !option.checked)
        .map(option => option.uid);
      const newOptionsStatuses: OptionStatus[] = optionsStatuses.map(option => {
        if (option.checked) {
          return {
            ...option,
            settingState: SettingState.Progress,
          };
        } else {
          return {
            ...option,
            disabled: true,
          };
        }
      });
      setOptionsStatuses(newOptionsStatuses);
    }

    setCommonState(SettingState.Progress)

    applyChanges([disabledUids])
  }, [optionsStatuses, applyChanges])

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
          commonState,
          siteName,
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
            onClick={api.closeDialog}
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
