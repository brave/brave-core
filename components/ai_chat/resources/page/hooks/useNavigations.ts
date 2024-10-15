// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect } from "react";
import { AIChatContext, useAIChat } from "../state/ai_chat_context";

const routes: { [key: string]: (context: AIChatContext, params: { [key: string]: string }) => Promise<void> } = {
    ['/']: async (context) => {
        context.onNewConversation()
    },
    ['/{chatId}']: async (context, params: { chatId: string }) => {
        if (context.visibleConversations.find(c => c.uuid === params.chatId)) {
            context.onSelectConversationUuid(params.chatId)
        } else {
            location.href = "/"
        }
    }
}

const getParamName = (part: string) => {
    if (part.startsWith("{") && part.endsWith("}")) {
        return part.substring(1, part.length - 1)
    }
    return null
}

const findMatchingRoute = (url: URL) => {
    const path = url.pathname
    const pathParts = path.split('/')


    for (const [path, handler] of Object.entries(routes)) {
        const routeParts = path.split('/')

        if (routeParts.length !== pathParts.length) continue

        let isMatch = true
        const params: { [key: string]: string } = {}
        for (let i = 0; i < pathParts.length; ++i) {
            const routePart = routeParts[i]
            const pathPart = pathParts[i]

            const paramName = getParamName(routePart)
            // If its a param, store the value
            if (paramName) {
                params[paramName] = pathPart
            } else if (routePart !== pathPart) {
                // The path doesn't match, so bail
                isMatch = false
                break
            }
        }

        if (isMatch) {
            return [handler, params] as const
        }
    }

    // Couldn't find a handler
    return [null, {}] as const
}

let initialized = false
export default function useNavigations() {
    const aiChat = useAIChat()
    useEffect(() => {
        const handler = (e?: NavigateEvent) => {
            const url = e?.destination.url ? new URL(e.destination.url) : new URL(window.location.href)
            const [handler, params] = findMatchingRoute(url)
            if (!handler) {
                return
            }

            if (e) {
                e.intercept({
                    handler: () => handler(aiChat, params)
                })
            } else {
                handler(aiChat, params)
            }
        }

        if (!initialized) {
            handler()
            initialized = true
        }
        window.navigation.addEventListener('navigate', handler)
        return () => {
            window.navigation.removeEventListener('navigate', handler)
        }
    }, [aiChat.visibleConversations])
}
