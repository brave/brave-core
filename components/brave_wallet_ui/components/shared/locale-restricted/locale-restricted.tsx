import * as React from 'react'

interface Props {
    allowedLocales: string[]
}

/**
 * Displays it's children to only users locale within a certain locale
 */
export function LocaleRestricted ({
    allowedLocales,
    children
}: React.PropsWithChildren<Props>) {
    return allowedLocales.includes(Intl.NumberFormat().resolvedOptions().locale)
        ? <>{children}</>
        : null
}
