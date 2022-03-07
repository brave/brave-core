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
