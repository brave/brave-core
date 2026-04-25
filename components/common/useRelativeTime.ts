// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect, useState } from "react"

// Set numeric to auto so we get 'yesterday' instead of '1 day ago'
const rtf = new Intl.RelativeTimeFormat(undefined, {
    numeric: 'auto'
})

const second = 1000
const minute = 60 * second
const hour = 60 * minute
const day = 24 * hour
const week = 7 * day
const year = 365 * day

const durations = {
    second, minute, hour, day, week, year
}

// Formats a time relative to now as the rounded number of units.
export const getRelativeTime = (time: Date | number) => {
    const now = Date.now()
    const then = new Date(time).getTime()
    const diff = then - now

    const [unit, duration] = (Object.entries(durations) as any).findLast(([, duration]: [string, number]) => Math.abs(diff) > duration) ?? ['second', 1000]
    const amount = Math.floor(diff / duration)
    return {
        formatted: rtf.format(amount, unit ),
        updatesIn: Math.max(10 * minute, duration - (Math.abs(diff - amount))),
    }
}

export default function useRelativeTime(time: Date | number) {
    const [formatted, setFormatted] = useState('')

    useEffect(() => {
        if (!time) return
        let timeout: NodeJS.Timeout
        const update = () => {
            const { formatted, updatesIn } = getRelativeTime(time)
            setFormatted(formatted)
            timeout = setTimeout(update, updatesIn)
        }

        update()

        return () => {
            clearTimeout(timeout)
        }
    }, [time])

    return formatted
}
