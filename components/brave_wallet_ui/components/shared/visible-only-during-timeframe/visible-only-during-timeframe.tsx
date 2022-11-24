// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

interface Props {
    startDate: Date
    endDate: Date
}

/**
 * Hides it's children when the current time is not within the provided range
 */
export function VisibleOnlyDuringTimeFrame ({
    endDate,
    startDate,
    children
}: React.PropsWithChildren<Props>) {
    const [now, setNow] = React.useState(new Date(Date.now()))

    const isVisible = React.useMemo(
        () => now >= startDate && now <= endDate, // time's up?
        [now, startDate, endDate]
    )

    // check every 60 seconds for current time
    React.useEffect(() => {
        const ref = setInterval(() => {
            setNow(new Date(Date.now()))
        }, 1000 * 60)

        // clear interval on unmount
        return () => {
            clearInterval(ref)
        }
    }, [])

    return isVisible ? <>{children}</> : null
}
