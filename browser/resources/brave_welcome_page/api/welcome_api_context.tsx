/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { WelcomeApi } from './welcome_api'
import generateReactContextForAPI from '$web-common/api/react_api'

export const { useAPI: useWelcomeApi, Provider: WelcomeApiProvider } =
  generateReactContextForAPI<WelcomeApi>()
