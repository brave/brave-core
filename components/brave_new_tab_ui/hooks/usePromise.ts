import { useEffect, useState } from 'react'

export default function usePromise<T> (getPromise: () => Promise<T>, deps: any[]) {
    const [result, setResult] = useState<T | undefined>()
    const [loading, setLoading] = useState(true)
    const [error, setError] = useState<any>()

    useEffect(() => {
        let cancelled = false

        setLoading(true)
        setError(undefined)

        getPromise()
            .then(result => !cancelled && setResult(result))
            .catch(err => !cancelled && setError(err))
            .finally(() => !cancelled && setLoading(false))

        return () => { cancelled = true }
    }, deps)

    return {
        result,
        loading,
        error
    }
}
