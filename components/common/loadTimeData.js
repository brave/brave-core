// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
if (!window.loadTimeData) {
  console.error('window.loadTimeData was not found. Make sure you include strings.m.js or load_time_data.m.js directly. Perhaps there is a timing issue?')
}
export const loadTimeData = window.loadTimeData
