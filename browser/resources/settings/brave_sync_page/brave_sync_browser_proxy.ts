// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js';
import { loadTimeData } from '../i18n_setup.js';
import { SyncStatus } from '/shared/settings/people_page/sync_browser_proxy.js';

export type BraveDeviceInfo = {
  name: string
  guid: string
  id: string
  os: string
  type: string
  chromeVersion: string
  lastUpdatedTimestamp: number
  sendTabToSelfReceivingEnabled: boolean
  supportsSelfDelete: boolean
  isCurrentDevice: boolean
  hasSharingInfo: boolean
}

export interface BraveSyncStatus extends SyncStatus {
  hasSyncWordsDecryptionError: boolean
  isOsEncryptionAvailable: boolean
}

export class BraveSyncBrowserProxy {
  getSyncCode(): Promise<string> {
    return sendWithPromise('SyncSetupGetSyncCode');
  }
  getPureSyncCode(): Promise<string> {
    return sendWithPromise('SyncSetupGetPureSyncCode');
  }
  getQRCode(syncCode: string): Promise<string> {
    return sendWithPromise('SyncGetQRCode', syncCode);
  }
  getDeviceList(): Promise<BraveDeviceInfo[]> {
    return sendWithPromise('SyncGetDeviceList');
  }
  setSyncCode(syncCode: string): Promise<boolean> {
    return sendWithPromise('SyncSetupSetSyncCode', syncCode);
  }
  resetSyncChain(): Promise<boolean> {
    return sendWithPromise('SyncSetupReset');
  }
  deleteDevice(deviceId: string): Promise<boolean> {
    return sendWithPromise('SyncDeleteDevice', deviceId);
  }
  getSyncStatus(): Promise<BraveSyncStatus> {
    return sendWithPromise('SyncSetupGetSyncStatus');
  }
  permanentlyDeleteSyncAccount(): Promise<boolean> {
    return sendWithPromise('SyncPermanentlyDeleteAccount');
  }
  getWordsCount(syncCode: string): Promise<number> {
    return sendWithPromise('SyncGetWordsCount', syncCode);
  }

  wasCustomSyncUrlEnabledAtStartup(): boolean {
    return loadTimeData.getBoolean('customSyncUrlEnabledAtStartup');
  }

  getCustomSyncUrlAtStartup(): string {
    return loadTimeData.getString('customSyncUrlAtStartup');
  }

  validateCustomSyncUrl(url: string) {
    return sendWithPromise('validateCustomSyncUrl', url);
  }

  static getInstance() {
    return instance || (instance = new BraveSyncBrowserProxy())
  }
}

let instance: BraveSyncBrowserProxy | null = null
