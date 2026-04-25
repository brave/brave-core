// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import {
  addWebUiListener,
  sendWithPromise
} from 'chrome://resources/js/cr.js';

export function submitReport(reportDetails: { [key: string]: any }) {
  chrome.send('webcompat_reporter.submitReport', [
    reportDetails
  ])
}

export function closeDialog() {
  chrome.send('dialogClose')
}

export function getDialogArgs(): string {
  return chrome.getVariableValue('dialogArguments')
}

export function captureScreenshot(): Promise<void> {
  return sendWithPromise('webcompat_reporter.captureScreenshot')
}

export function clearScreenshot() {
  chrome.send('webcompat_reporter.clearScreenshot')
}

export function getCapturedScreenshot(): Promise<string> {
  return sendWithPromise('webcompat_reporter.getCapturedScreenshot')
}

export interface ViewPortSizeChangedObject {
  height: number
}

export function setViewPortChangeListener(
  calback: (data: ViewPortSizeChangedObject) => void) {
  sendWithPromise('webcompat_reporter.init').then(calback)
  addWebUiListener('onViewPortSizeChanged', calback)
}