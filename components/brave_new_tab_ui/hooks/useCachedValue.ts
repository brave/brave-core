import { useCallback, useEffect, useState } from 'react'

export const useCachedValue = <T>(value: T, setValue: (newValue: T) => void) => {
    const [cached, setCached] = useState<T>(value)
    const updateCached = useCallback((newValue: T) => {
        setCached(newValue)
        setValue(newValue)
    }, [setValue])

    useEffect(() => {
        setCached(value)
    }, [value])

    return [cached, updateCached] as const
}
