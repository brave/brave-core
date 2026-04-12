// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Temporary type definitions for PSST mojom interfaces
// These match the definitions in psst_ui_common.mojom and will be replaced
// by generated types when the build system creates the actual mojom files

export enum PsstStatus {
  kInProgress = 0,
  kCompleted = 1,
  kFailed = 2,
}

export interface SettingCardDataItem {
  description: string
  url: string
}

export interface SettingCardData {
  siteName: string
  items: SettingCardDataItem[]
}

export interface PsstConsentHelperRemote {
  applyChanges(siteName: string, disabledSettingsList: string[]): Promise<void>
  closeDialog(): Promise<void>
  proxy: any
  onConnectionError: { addListener: (callback: () => void) => void }
  $: {
    bindNewPipeAndPassReceiver: () => any
  }
}

export interface PsstConsentDialogCallbackRouter {
  setSettingsCardData: {
    addListener: (callback: (data: SettingCardData) => void) => void
  }
  onSetRequestDone: {
    addListener: (callback: (url: string, error?: string) => void) => void
  }
  onSetCompleted: {
    addListener: (
      callback: (appliedChecks?: string[], errors?: string[]) => void,
    ) => void
  }
  $: {
    bindNewPipeAndPassRemote: () => any
  }
}

export interface PsstConsentFactory {
  getRemote(): {
    createPsstConsentHandler(receiver: any, remote: any): void
  }
}

// Concrete implementations of the interfaces for development/testing
export class PsstConsentHelperRemote implements PsstConsentHelperRemote {
  async applyChanges(
    siteName: string,
    disabledSettingsList: string[],
  ): Promise<void> {
    console.log('PsstConsentHelperRemote.applyChanges:', {
      siteName,
      disabledSettingsList,
    })
  }

  async closeDialog(): Promise<void> {
    console.log('PsstConsentHelperRemote.closeDialog called')
  }

  proxy: any = {}

  onConnectionError = {
    addListener: (callback: () => void) => {
      console.log(
        'PsstConsentHelperRemote.onConnectionError.addListener called',
      )
    },
  }

  $ = {
    bindNewPipeAndPassReceiver: (): any => {
      return {} as any
    },
  }
}

export class PsstConsentDialogCallbackRouter
  implements PsstConsentDialogCallbackRouter
{
  private settingsCardDataListeners: Array<(data: SettingCardData) => void> = []
  private setRequestDoneListeners: Array<
    (url: string, error?: string) => void
  > = []
  private setCompletedListeners: Array<
    (appliedChecks?: string[], errors?: string[]) => void
  > = []

  setSettingsCardData = {
    addListener: (callback: (data: SettingCardData) => void) => {
      this.settingsCardDataListeners.push(callback)
    },
  }

  onSetRequestDone = {
    addListener: (callback: (url: string, error?: string) => void) => {
      this.setRequestDoneListeners.push(callback)
    },
  }

  onSetCompleted = {
    addListener: (
      callback: (appliedChecks?: string[], errors?: string[]) => void,
    ) => {
      this.setCompletedListeners.push(callback)
    },
  }

  $ = {
    bindNewPipeAndPassRemote: (): any => {
      return {} as any
    },
  }

  // Helper methods to trigger callbacks (for testing/development)
  _triggerSettingsCardData(data: SettingCardData) {
    this.settingsCardDataListeners.forEach((callback) => callback(data))
  }

  _triggerSetRequestDone(url: string, error?: string) {
    this.setRequestDoneListeners.forEach((callback) => callback(url, error))
  }

  _triggerSetCompleted(appliedChecks?: string[], errors?: string[]) {
    this.setCompletedListeners.forEach((callback) =>
      callback(appliedChecks, errors),
    )
  }
}

export class PsstConsentFactoryImpl implements PsstConsentFactory {
  getRemote() {
    return {
      createPsstConsentHandler(receiver: any, remote: any): void {
        console.log('PsstConsentFactory.createPsstConsentHandler called:', {
          receiver,
          remote,
        })
      },
    }
  }
}

// Export a factory instance
export const PsstConsentFactory = new PsstConsentFactoryImpl()
