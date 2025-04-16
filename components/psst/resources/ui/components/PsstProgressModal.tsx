/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ModalTitle } from './basic/display'
import { Container, HorizontalContainer, LeftAlignedItem, PsstDlgButton, RightAlignedItem, TextSection } from './basic/structure'
import SettingsCard from './SettingsCard'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { BravePsstConsentDialogProxy, SettingCardData } from '../browser_proxy'
import { getLocalizedString } from '../lib/locale_context'

 export interface Props {
 }

  export enum SettingState {
    None,
    Selection,
    Progress,
    Completed,
    Failed
  }

  interface OptionStatus {
    url: string,
    description: string,
    error: string | null,
    checked: boolean,
    disabled: boolean,
    settingState: SettingState
  }

  export interface PsstProgressModalState {
    commonState: SettingState
    site_name: string,
    optionsStatuses: Map<string, OptionStatus> | null 
  }

 export default class PsstProgressModal extends React.PureComponent<Props, PsstProgressModalState> {
    constructor (props: Props) {
      super(props)
      this.state = {
        commonState: SettingState.None,
        site_name: '',
        optionsStatuses: new Map(),
      }
    }

    browserProxy_: BravePsstConsentDialogProxy =
    BravePsstConsentDialogProxy.getInstance()
    
    closeDialog = () => this.browserProxy_.getPsstConsentHelper().closeDialog()

    setStateProp = (
      updates: Partial<OptionStatus>,
      predicate: (status: OptionStatus) => boolean = () => true
    ) => {
      this.setState((prevState) => {
        if (!prevState.optionsStatuses) {
          return prevState;
        }
    
        const updatedOptionsStatuses = new Map<string, OptionStatus>();
        prevState.optionsStatuses.forEach((status, key) => {
          if (predicate(status)) {
            updatedOptionsStatuses.set(key, { ...status, ...updates });
          } else {
            updatedOptionsStatuses.set(key, status);
          }
        });
    
        return { ...prevState, optionsStatuses: updatedOptionsStatuses };
      });
    };

    setPropForUrl = (targetUrl: string, updates: Partial<OptionStatus>) => {
      this.setState((prevState) => {
        if (!prevState.optionsStatuses) {
          return prevState;
        }
    
        const updatedOptionsStatuses = new Map<string, OptionStatus>();
        let found = false;
    
        prevState.optionsStatuses.forEach((status, key) => {
          if (status.url === targetUrl) {
            updatedOptionsStatuses.set(key, { ...status, ...updates });
            found = true;
          } else {
            updatedOptionsStatuses.set(key, status);
          }
        });
    
        if (found) {
          return { ...prevState, optionsStatuses: updatedOptionsStatuses };
        }
        return prevState;
      });
    };

    componentDidMount () {
      this.browserProxy_.getCallbackRouter().setSettingsCardData.addListener((scd: SettingCardData) => {
        const checkedUrlsMap = new Map<string, OptionStatus>()
        scd.items.forEach((item) => {
          checkedUrlsMap.set(item.url, { 
              url: item.url, 
              description: item.description,
              error: null,
              checked: true,
              disabled: false,
              settingState: SettingState.Selection
          }) 
        })
        this.setState({
          site_name: scd.siteName,
          optionsStatuses: checkedUrlsMap
        })
      })
      this.browserProxy_.getCallbackRouter().onSetRequestDone.addListener((url: string, error: string | null) => {
        this.setPropForUrl(url, { settingState: error ? SettingState.Failed : SettingState.Completed, error: error })
      })
      this.browserProxy_.getCallbackRouter().onSetCompleted.addListener((url: string[], errors: string[]) => {
        this.setState({ commonState: SettingState.Completed });
      })
    }

    handleSettingItemCheck = (url: string, checked: boolean) => {
      this.setState(prevState => {
        const os = prevState?.optionsStatuses?.get(url)
        if(os){
          prevState?.optionsStatuses?.set(url, { 
            checked: checked,
            url: os.url,
            description: os.description,
            error: os.error,
            disabled: os.disabled,
            settingState: os.settingState
          })
        }
      })
    }

    render () {
        const isInProgress = this.state.commonState == SettingState.Progress
        return (
        <Container>
          <HorizontalContainer>
            <LeftAlignedItem>
              <TextSection>
                <ModalTitle>
                  {getLocalizedString('bravePsstDialogTitle')}
                </ModalTitle>
              </TextSection>
            </LeftAlignedItem>
            <RightAlignedItem>
              <Button fab kind='plain-faint'
                isDisabled={isInProgress}
                onClick={() => this.browserProxy_.getPsstConsentHelper().closeDialog()}>
                <Icon name={"close-circle"}/>
              </Button>
            </RightAlignedItem>
          </HorizontalContainer>
          <TextSection>
              {getLocalizedString('bravePsstDialogTitle')}
          </TextSection>
           <SettingsCard title={getLocalizedString('bravePsstDialogOptionsTitle')} 
              subTitle={this.state.site_name}
              progressModelState={this.state}
              onItemChecked={this.handleSettingItemCheck} />
          {(() => {
            if(this.state.commonState != SettingState.Completed) {
              return (              
                <RightAlignedItem>
                    <PsstDlgButton
                      kind='outline'
                      size='medium'
                      isDisabled={isInProgress}
                      onClick={this.closeDialog}
                    >
                      {getLocalizedString('bravePsstDialogCancelBtn')}
                    </PsstDlgButton>
                    <PsstDlgButton
                      kind='filled'
                      size='medium'
                      isDisabled={isInProgress}
                      isLoading={isInProgress}
                      onClick={() => {
                        let settingsToProcess: string[] = [];
                        if(this.state.optionsStatuses) {
                          settingsToProcess = Array.from(this.state.optionsStatuses?.entries()).filter(([_, value]) => !value.checked).map(([key]) => key)
                        }
                        this.setStateProp({ settingState: SettingState.Progress }, (status) => status.checked)
                        this.setStateProp({ disabled: true }, (status) => !status.checked)
                        this.setState({ commonState: SettingState.Progress });
                        this.browserProxy_.getPsstConsentHelper().applyChanges(settingsToProcess)
                      }}
                    >
                      {getLocalizedString('bravePsstDialogOkBtn')}
                    </PsstDlgButton>
                </RightAlignedItem>)
            } else {
              return (              
                <RightAlignedItem>
                    <PsstDlgButton
                      kind='outline'
                      size='medium'
                      isDisabled={false}
                      onClick={this.closeDialog}
                    >
                      {getLocalizedString('bravePsstDialogReportFailedBtn')}
                    </PsstDlgButton>
                    <PsstDlgButton
                      kind='filled'
                      size='medium'
                      isDisabled={false}
                      isLoading={false}
                      onClick={this.closeDialog}
                    >
                      {getLocalizedString('bravePsstDialogCloseBtn')}
                    </PsstDlgButton>
                </RightAlignedItem>)
            }
          })()}

        </Container>)
    }
}