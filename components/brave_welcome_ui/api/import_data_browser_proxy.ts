// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Copied from Chromium

// clang-format off
import { sendWithPromise } from 'chrome://resources/js/cr.js'
// clang-format on

/**
 * An object describing a source browser profile that may be imported.
 * The structure of this data must be kept in sync with C++ ImportDataHandler.
 */
export interface BrowserProfile {
  name: string
  index: number
  profileName: string
  history: boolean
  favorites: boolean
  passwords: boolean
  search: boolean
  autofillFormData: boolean
}

/**
 * These string values must be kept in sync with the C++ ImportDataHandler.
 */
export enum ImportDataStatus {
  INITIAL = 'initial',
  IN_PROGRESS = 'inProgress',
  SUCCEEDED = 'succeeded',
  FAILED = 'failed',
}

export interface ImportDataBrowserProxy {
  /**
   * Returns the source profiles available for importing from other browsers.
   */
  initializeImportDialog: () => Promise<BrowserProfile[]>

  /**
   * Starts importing data for the specified source browser profile. The C++
   * responds with the 'import-data-status-changed' WebUIListener event.
   * @param types Which types of data to import.
   */
  importData: (
      sourceBrowserProfileIndex: number,
      types: { [type: string]: boolean }) => void

  /**
   * Starts importing data for the specified source browser profiles. The C++
   * responds with the 'import-data-status-changed' WebUIListener event.
   * @param types Which types of data to import.
   */
  importDataBulk: (
    sourceBrowserProfileIndex: number[],
    types: { [type: string]: boolean }) => void

  /**
   * Prompts the user to choose a bookmarks file to import bookmarks from.
   */
  importFromBookmarksFile: () => void
}

export class ImportDataBrowserProxyImpl implements ImportDataBrowserProxy {
  initializeImportDialog () {
    return sendWithPromise('initializeImportDialog')
  }

  importData (
      sourceBrowserProfileIndex: number, types: { [type: string]: boolean }) {
    chrome.send('importData', [sourceBrowserProfileIndex, types])
  }

  importDataBulk (
      sourceBrowserProfileIndex: number[], types: { [type: string]: boolean }) {
    chrome.send('importDataBulk', [sourceBrowserProfileIndex, types])
  }

  importFromBookmarksFile () {
    chrome.send('importFromBookmarksFile')
  }

  static getInstance (): ImportDataBrowserProxy {
    return instance || (instance = new ImportDataBrowserProxyImpl())
  }

  static setInstance (obj: ImportDataBrowserProxy) {
    instance = obj
  }
}

let instance: ImportDataBrowserProxy | null = null
