/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

declare namespace WebcompatReporter {
  export interface ApplicationState {
    reporterState: State | undefined
  }

  export interface DialogArgs {
    url: string
    isErrorPage: boolean
    adBlockSetting: string
    fpBlockSetting: string
    shieldsEnabled: string
    contactInfo: string
    contactInfoSaveFlag: boolean
  }

  export interface State {
    dialogArgs: DialogArgs
    submitted: boolean
  }
}
