// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import { App } from './components/app'
import './strings'
import {
  PageCallbackRouter,
  PageHandlerFactory,
  PageHandlerRemote,
} from 'gen/brave/components/desktop_wallpaper/desktop_wallpaper.mojom.m.js'

setIconBasePath('chrome://resources/brave-icons')

export const handler = new PageHandlerRemote()
export const callbackRouter = new PageCallbackRouter()

PageHandlerFactory.getRemote().createPageHandler(
  callbackRouter.$.bindNewPipeAndPassRemote(),
  handler.$.bindNewPipeAndPassReceiver(),
)

createRoot(document.getElementById('root')!).render(<App />)
