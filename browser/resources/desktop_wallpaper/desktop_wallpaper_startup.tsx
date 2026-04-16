import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import { App } from './components/app'
import './strings'
import {
  PageCallbackRouter,
  PageHandlerFactory,
  PageHandlerRemote,
} from 'gen/brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.m.js'

setIconBasePath('chrome://resources/brave-icons')

const handler = new PageHandlerRemote()
const callbackRouter = new PageCallbackRouter()

PageHandlerFactory.getRemote().createPageHandler(
  callbackRouter.$.bindNewPipeAndPassRemote(),
  handler.$.bindNewPipeAndPassReceiver()
)

createRoot(document.getElementById('root')!).render(<App />)
