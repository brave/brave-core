// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  "use strict"

  /**
   * Send the results of farbled APIs to iOS so it can see if its properly farbled
   * @param {Array} voiceNames An array of voice names from the SpeechSynthesis API
   * @param {number} hardwareConcurrency A value of the hardwareConcurrency API
   * @param {Array} pluginNames An array of voice names from the Plugin API
   * @returns A Promise that resolves new hide selectors
   */
  const sendTestResults = (voiceNames, hardwareConcurrency, pluginNames) => {
    return webkit.messageHandlers['SendTestFarblingResult'].postMessage({
      'voiceNames': voiceNames,
      'hardwareConcurrency': hardwareConcurrency,
      'pluginNames': pluginNames
    })
  }

  const voices = window.speechSynthesis.getVoices()
  const voiceNames = voices.map(x => x.name)
  const hardwareConcurrency = window.navigator.hardwareConcurrency

  const pluginNames = []
  for (let i = 0; i < window.navigator.plugins.length; i++) {
    let name = window.navigator.plugins[i].name
    pluginNames.push(name)
  }

  sendTestResults(voiceNames, hardwareConcurrency, pluginNames)
})()
