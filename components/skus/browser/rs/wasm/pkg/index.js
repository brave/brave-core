// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import load, { initialize as init } from "./skus_sdk.js";

let wasmImportURL = undefined

export const initialize = async function() {
  await load(wasmImportURL)
  return await init.apply(this, arguments)
}

export const setWASMImportURL = function(url) {
  wasmImportURL = url
}
