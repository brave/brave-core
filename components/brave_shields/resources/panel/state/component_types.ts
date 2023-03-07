// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { Url } from 'gen/url/mojom/url.mojom.m.js'

export enum ViewType {
  ScriptsList,
  AdsList,
  HttpsList,
  Main
}

export enum ResourceType {
  Script,
  Ad,
  Http
}

export enum ResourceState {
  Blocked,
  AllowedOnce
}

export type ResourceInfo = {
  url: Url
  state: ResourceState
  type: ResourceType
}

export function makeResourceInfoList (data: Url[],
                                      type: ResourceType,
                                      state: ResourceState): ResourceInfo[] {
  return data.map(entry => ({ url: entry, type, state }))
}

export type PermissionButtonHandler =
  ((name: string, state: ResourceState) => void) | undefined
