// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useNavigation, useParams } from '$web-common/navigation/Context'
import { useEffect } from 'react'

export const tabAssociatedChatId = 'tab'

export function useSelectedConversation() {
    return useParams().chatId
}

const routes = [
    '/',
    '/{chatId}'
]

export function Routes() {
    const { addRoute, removeRoute } = useNavigation()
    useEffect(() => {
        for (const route of routes) {
            addRoute(route)
        }

        return () => {
            for (const route of routes) {
                removeRoute(route)
            }
        }
    }, [])

    return null
}
