// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
