// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  QueryClient,
  QueryObserver,
  useQuery,
  UseQueryResult as ReactQueryUseQueryResult,
  useMutation,
  UseMutationResult as ReactQueryUseMutationResult,
  UseMutationOptions,
  QueryObserverOptions,
  MutationOptions,
  MutationObserver,
  MutateOptions,
} from '@tanstack/react-query'

//
// This file contains a factory function, createMojoAPI, to create a
// subscribable API with a shared cache based off a mojom interface which
// exposes features of Tanstack Query in a similar fashion to
// RTK Query's createApi.
//
// Whilst this sets up a store that could be (ab)used to store, update and subscribe
// to any data, it encourages its use only for remote-fetched data,
// and not reactive state needed for the UI.
// The intention is for that kind of state to be handled by the
// more featureful UI framework
//
// Things you can provide and what they do:
//
// ENDPOINTS:
//
// { endpoints: { myEndpoint: { query: (arg1, arg2) => Promise<Result> } } }
// - Creates api.myEndpoint.fetch(arg1, arg2) to manually call and update the cache
// - Creates api.useMyEndpoint(arg1, arg2) to create a React Query hook that fetches the data
//
// { endpoints: { myEndpoint: { mutation: (arg1, arg2) => Promise<Result> } } }
// - Creates api.myEndpoint.mutate(arg1, arg2) to manually call and update the cache
// - Creates api.useMyEndpoint() to create a React Query hook that runs the mutation
//   only when .mutate() is called.
//
// ACTIONS:
//
// { actions: { myAction: (arg1, arg2) => void } }
// - Creates api.myAction(arg1, arg2) to call the action directly. Does not cache.
//
// EVENTS:
//
// { events: { onMyEvent: event<[arg1, arg2], Payload>() } }
// - Creates api.emitEvent('onMyEvent', arg1, arg2, payload) to emit an event
// - Creates api.useOnMyEvent(arg1, arg2) to create a React Query hook that subscribes
//   to the latest payload for that event
// - Creates api.handledOnMyEvent(arg1, arg2) to reset the event data for that key
//   so that subscribers will no longer receive the payload.
//

type ChangeReturnType<T extends (...args: any[]) => any, R> =
  T extends (...args: infer P) => any
    ? (...args: P) => R
    : never;

export type NoPlaceholderQueryEndpointDefinition<
  P extends readonly any[],
  R,
> = {
  query: (...args: P) => Promise<R>
  prefetchWithArgs?: NoInfer<P>
} & Omit<
  QueryObserverOptions<NoInfer<R>>,
  'queryKey' | 'queryFn' | 'placeholderData'
>

export type PlaceholderQueryEndpointDefinition<
  P extends readonly any[],
  R,
> = NoPlaceholderQueryEndpointDefinition<P, R> & {
  placeholderData: NoInfer<R>
}

export type QueryEndpointDefinition<P extends readonly any[], R> =
  | NoPlaceholderQueryEndpointDefinition<P, R>
  | PlaceholderQueryEndpointDefinition<P, R>

export type MutationEndpointDefinition<P extends readonly any[], R> = {
  mutation: (...args: P) => Promise<R>
} & Omit<MutationOptions<NoInfer<R>, unknown, P>, 'mutationKey' | 'mutationFn'>

/**
 * Each endpoint must be one of:
 *   { query: (...args: A) => Promise<T>, prefetch?: boolean }
 * or
 *   { mutation: (...args: A) => Promise<T> }
 *
 * For query endpoints, you get `.fetch(...)`, `.useQuery(...)`, `.invalidate(...)`, and `.update(...)`.
 * For mutation endpoints, you get `.mutate(...)` and `.useMutation()`.
 */
export type EndpointDef<P extends readonly any[], R> =
  | QueryEndpointDefinition<P, R>
  | MutationEndpointDefinition<P, R>

/**
 * EventDef<Args, Payload> is just a phantom object whose sole job is to carry
 * the tuple‐type `Args` (an array of key‐argument types) and the `Payload` type.
 *
 * At runtime `event()` returns a dummy object; we only use its type for inference.
 */
export type EventDef<Args extends any[], Payload extends any[]> = {
  registerEmitter: (emitter: (...args: Payload) => void) => void
  /** phantom field so we can extract `Args` at the type level */
  __args: Args
}

// This function only exists to actually prevent inferring of
// query args from prefetchWithArgs so that typescript avoids
// confusion if the query() parameters don't match the prefetchWithArgs type
// and informs the consumer that there's a mismatch that needs to be fixed.
// Ideally consumers would not need to wrap their endpoints in query()
// and they don't if they don't define prefetchWithArgs.
export function query<const P extends readonly any[], R>(
  endpoint: QueryEndpointDefinition<P, R>,
): QueryEndpointDefinition<P, R> {
  return endpoint
}

/**
 * Wrapper for an endpoint which creates data that is not fetched but is
 * updated by events and provided with initial data.
 * @param data optional initial data (not placeholder data)
 * @returns query endpoint definition
 */
export function state<R>(data?: R): QueryEndpointDefinition<[], R> {
  return {
    query: () =>
      Promise.reject(new Error('State endpoint should not be called')),
    enabled: false,
    prefetchWithArgs: [],
    // TODO(petemill): should be placeholderData?
    initialData: data,
    // never re-fetch, even if invalidated
    staleTime: 'static',
  }
}

/**
 * Call this for each event in your config. For example:
 *   events: {
 *     onConversationDeleted: event<[string], boolean>(),
 *     onUserRenamed:       event<[number], { id: number; newName: string }>(),
 *   }
 *
 *   That tells the factory "`onConversationDeleted` takes one key‐arg (string),
 *   and its payload is a boolean."
 */
export function event<
  Args extends any[], Payload extends any[]
>(registerEmitter: (emitter: (...args: Payload) => void) => void): EventDef<Args, Payload> {
  return {
    registerEmitter,
    __args: [] as any as Args,
  }
}

export function createInterfaceApi<
  /**
   * ExposedActions is a record of simple actions that can be called directly from the UI.
   * These actions do not require any status helpers or long-term data caching.
   * For example:
   *   {
   *     sendMessage: (message: string) => void,
   *     deleteConversation: (id: string) => void,
   *     …
   *   }
   */
  const ExposedActions extends Record<string, ((...args: readonly any[]) => any) | Record<string, (...args: readonly any[]) => any>>,
  const RawEndpoints extends Record<string, EndpointDef<readonly any[], any>>,
  EventDefinitions extends Record<string, EventDef<any[], any>>,
>(config: {
  /** Unique identifier for this API, used in query keys */
  key?: string

  /**
  * Simple actions that can be called directly from the UI. These
  * don't need any status helpers or long-term data caching. If you helper
  * data for status would be useful, consider using a mutation endpoint instead.
  * TODO(petemill): make optional
  */
  actions: ExposedActions

  /** Actions to expose to the UI, each have a result that will be cached or mutated */
  endpoints: RawEndpoints

  /** An array of event names to broadcast */
  events?: EventDefinitions,

  /**
   * Provide a queryClient to preserve the cache between multiple invocations
   * of the same type of API. Ensure that you provide a key.
   */
  queryClient?: QueryClient,
}) {
  // Validate
  if (config.queryClient && !config.key) {
    console.warn('createInterfaceApi received a queryClient as if it were to be used for multiple instances, but no key with which to differentiate them. This may cause cache collisions.')
  }

  const queryClient = config.queryClient ?? new QueryClient()

  type ValidKey = keyof RawEndpoints & string

  // For each endpoint K:
  //   ArgsOf<K> = the parameter‐tuple of raw[K].[query|mutation]
  //   DataOf<K> = the (Promise-d) returned type of raw[K].[query|mutation]
  type ArgsOf<K extends ValidKey> =
    RawEndpoints[K] extends QueryEndpointDefinition<infer QueryArgs, any>
      ? QueryArgs
      : RawEndpoints[K] extends MutationEndpointDefinition<
            infer MutationArgs,
            any
          >
        ? MutationArgs
        : never
  type DataOf<K extends keyof RawEndpoints> =
    RawEndpoints[K] extends QueryEndpointDefinition<any, infer QueryResult>
      ? QueryResult
      : RawEndpoints[K] extends MutationEndpointDefinition<
            any,
            infer MutationResult
          >
        ? MutationResult
        : never

  // If placeholder data is provided, we can assume that data is never undefined
  type EndpointDataForKey<K extends ValidKey> =
    RawEndpoints[K] extends { placeholderData: DataOf<K> }
      ? DataOf<K>
      : DataOf<K> | undefined

  // Custom results
  type BaseUseQueryResult<K extends ValidKey> = ReactQueryUseQueryResult<
    DataOf<K>
  >
  type UseQueryResult<K extends ValidKey> = BaseUseQueryResult<K> & {
    // Convenience - a nicer name for data
    // *and* never undefined if placeholder data is provided, e.g.
    // `const { getThingsData } = useGetThings()`
    // instead of
    // ```
    // const { data: getThingsData } = useSaveSomething()
    // if (!getThingsData) {
    //   throw new Error('getThingsData should never be undefined as it has placeholder data')
    // }
    // ```
    [P in K as `${P}Data`]: EndpointDataForKey<K>
    // TODO(petemill): We could add more convenience properties here,
    // e.g. `isMyQueryLoading`.
  } & {
    // Never undefined if placeholderData is provided
    data: EndpointDataForKey<K>
  }

  type BaseUseMutationResult<K extends ValidKey> = ReactQueryUseMutationResult<
    DataOf<K>,
    unknown,
    ArgsOf<K>
  > & {
    // Nicer version of mutate where args are optional if no parameters
    mutate: ArgsOf<K> extends [] ? () => ReturnType<ReactQueryUseMutationResult<DataOf<K>, unknown, ArgsOf<K>>['mutate']> : ReactQueryUseMutationResult<DataOf<K>, unknown, ArgsOf<K>>['mutate']
  }
  type UseMutationResult<K extends ValidKey> = BaseUseMutationResult<K> & {
    // Convenience - a nicer name for mutate, e.g.
    // `const { saveSomething } = useSaveSomething()`
    // instead of
    // `const { mutate: saveSomething } = useSaveSomething()`
    [P in K]: BaseUseMutationResult<K>['mutate']
  }

  type APIActions = {
    [K in keyof ExposedActions]: ExposedActions[K] extends (...args: infer P) => any
      ? ExposedActions[K]
      : {
          [P in keyof ExposedActions[K]]: ExposedActions[K][P]
        }
  }

  // Don't allow individual hook uses to specify events since those are specified either at the endpoint
  // definition or by the mutation function call.
  type APIUseMutationOptions<K extends ValidKey> = Omit<UseMutationOptions<DataOf<K>, unknown, ArgsOf<K>>, 'onMutate' | 'onError' | 'onSuccess' | 'onSettled'>

  // Build out methods for every K in RawEndpoints, depending on whether it's a query or mutation
  type APIEndpoints = {
    [K in ValidKey]: RawEndpoints[K] extends QueryEndpointDefinition<any, any>
      ? {
          /**
           * Call the underlying query and return the data. This will
           * also cause anyone subscribed to this endpoint to receive the
           * updated data.
           */
          fetch: (...args: ArgsOf<K>) => Promise<DataOf<K>>

          /**
           * Imperative (non-reactive) way to retrieve data for this endpoint.
           * Should only be used in callbacks or functions where reading the
           * latest data is necessary, e.g. for optimistic updates.
           *
           * Hint: Do not use this function inside a component, because it won't
           * receive updates. Use useQuery to create a QueryObserver that
           * subscribes to changes.
           */
          current: (...args: ArgsOf<K>) => EndpointDataForKey<K>

          /**
           * Reset the cache for this endpoint
           * (specifying optional arguments as the key)
           * which will remove the data from the cache
           * and reset the state to undefined or placeholderData.
           */
          reset: (...args: ArgsOf<K>) => void

          /**
           * Invalidate the cache for this endpoint
           * (specifying optional arguments as the key to invalidate)
           * which will force a re-fetch of the data if currently
           * subscribed to, or on the next call to `fetch()`.
           */
          invalidate: (...args: ArgsOf<K>) => void

          /**
           * React hook for accessing state of a query endpoint including progress, cached data and re-fetching
           * See https://tanstack.com/query/latest/docs/framework/react/reference/useQuery
           */
          useQuery: (...args: ArgsOf<K>) => UseQueryResult<K>

          /**
           * Manually update the cache for this endpoint
           * (specifying optional arguments as the key to update).
           * Usage: api.myEndpoint.update(arg1, arg2, ..., updaterFnOrUpdate)
           * where updaterFnOrUpdate has signature (old: Data | undefined) => Data
           * OR is a Partial<Data> to update the object directly.
           *
           * This is useful in event handlers from the remote interface, or
           * optimistic updates.
           */
          update: (
            ...argsAndUpdater: [
              ...Params: ArgsOf<K>,
              updater:
                | ((old: DataOf<K>) => Partial<DataOf<K>>)
                | Partial<DataOf<K>>,
            ]
          ) => void
        } &
        (RawEndpoints[K] extends { placeholderData: DataOf<K> }
          ? {
              // If placeholder data is provided, we know
              // the data will never be undefined, so we can
              // provide a potentially even more convenient hook.
              useData: (...args: ArgsOf<K>) => DataOf<K>
            }
          : {})
      : RawEndpoints[K] extends MutationEndpointDefinition<any, any>
        ? {
            /**
             * Call the underlying `raw[K].mutation(...args)` and return the data.
             */
            mutate: ChangeReturnType<BaseUseMutationResult<K>['mutate'], Promise<DataOf<K>>>

            /**
             * Hook: runs `useMutation` for this mutation endpoint.
             * You can call `mutate(variables)` or `mutateAsync(variables)` on the result.
             */
            useMutation: (
              options?: APIUseMutationOptions<K>,
            ) => UseMutationResult<K>
          }
        : never
  }

  const endpoints = {} as APIEndpoints

  // Build endpoints - Queries and Mutations
  //
  // Expose React Query's hooks and methods for each endpoint,
  // deciding if it's a query or mutation endpoint.
  //
  //
  ;(Object.keys(config.endpoints) as Array<ValidKey>).forEach((name) => {
    const endpointDef = config.endpoints[name]
    const baseKey = config.key ? [config.key, name] as const : [name] as const

    if ('query' in endpointDef) {
      const { query, prefetchWithArgs, ...queryOptions } = endpointDef as QueryEndpointDefinition<any, any>
      type QArgs = ArgsOf<typeof name>
      type QData = DataOf<typeof name>

      if (!queryOptions.staleTime) {
        queryOptions.staleTime = Infinity
      }

      // fetch(...args):
      //   - calls `query(...args)`
      //   - sets the result into cache under [...baseKey, ...args]
      //   - returns the data
      const fetcher = async (...args: QArgs) => {
        const data = await queryClient.fetchQuery<QData, never, QArgs>({
          queryKey: [...baseKey, ...args],
          queryFn: () => {
            const queryResult = query(...args)
            return queryResult
          },
          ...queryOptions,
        })
        return data
      }

      const getCachedData = (...args: QArgs) => {
        const queryKey = [
          ...baseKey,
          ...args,
        ] as readonly any[]
        let cachedData = queryClient.getQueryData<QData>(queryKey)
        if (cachedData === undefined && 'placeholderData' in queryOptions) {
          cachedData = queryOptions.placeholderData
        }
        return cachedData
      }

      // invalidates the cache so that it will be refetched
      // if the data is needed.
      const invalidate = (...args: QArgs) => {
        const queryKey = [...baseKey, ...args]
        queryClient.invalidateQueries({
          queryKey,
          exact: true,
        })
      }

      // Removes state and resets to undefined or initialData
      const reset = (...args: QArgs) => {
        const queryKey = [...baseKey, ...args]
        // We can use removeQueries if we don't want
        // to reset to initialData.
        queryClient.resetQueries({
          queryKey,
          exact: true,
        })
      }

      // useQuery(...args): wrapper around React-Query's useQuery
      const useQ = (...args: QArgs) => {
        const useQueryResult = useQuery<QData>({
          queryKey: [...baseKey, ...args],
          queryFn: () => {
            const queryResult = query(...args)
            return queryResult
          },
          ...queryOptions,
        }, queryClient)
        return {
          ...useQueryResult,
          // Typescript will handle whether this property
          // is accessible as definitely QData or undefined | QData.
          // We don't need to check if placeholderData is provided.
          [`${name}Data`]: useQueryResult.data,
        } as UseQueryResult<typeof name>
      }

      // update(...args, updaterFn): manually setQueryData for that key
      // We can accept a partial update only if the data is an object
      type AllowedUpdateParam = QData extends {} ? Partial<QData> : QData
      function updateFromOld(old: QData | undefined, update: AllowedUpdateParam): QData {
        if (old === undefined || Array.isArray(old) || typeof old !== 'object') {
          // Technically we shouldn't allow this as it might just be partial
          return update as QData
        }
        // Objects can be combined
        return { ...old, ...update as Partial<QData> } as QData
      }

      const updater = (
        ...argsAndUpdater: [
          ...Params: QArgs,
          updaterFn: ((old: QData) => AllowedUpdateParam) | AllowedUpdateParam,
        ]
      ) => {
        const up = argsAndUpdater[argsAndUpdater.length - 1] as
          | ((old: QData) => AllowedUpdateParam)
          | Partial<QData>
        // args is everything except the updater function (or updated state) which is
        // always the last argument.
        const args = argsAndUpdater.slice(
          0,
          argsAndUpdater.length - 1,
        ) as unknown as QArgs

        queryClient.setQueryData<QData>([...baseKey, ...args], (old) => {
          const updateData: AllowedUpdateParam = (typeof up === 'function')
            ? up(old as QData)
            : up as AllowedUpdateParam

          const newData = updateFromOld(old, updateData)
          return newData
        })
      }

      // Set initial data to avoid a query
      if (queryOptions.initialData !== undefined && queryOptions.enabled === false) {
        queryClient.setQueryData(baseKey, () => queryOptions.initialData)
      }

      if (prefetchWithArgs && queryOptions.enabled !== false) {
        // If this endpoint is marked as prefetch, we prefetch it immediately
        // even if the UI doesn't call it yet - we don't
        // want to wait for React to initialize.
        fetcher(...(prefetchWithArgs as QArgs))
      }

      ;(endpoints as any)[name] = {
        fetch: fetcher,
        current: getCachedData,
        invalidate,
        reset,
        useQuery: useQ,
        update: updater,
      }
    } else if ('mutation' in endpointDef) {
      const { mutation, ...mutationOptions } = endpointDef
      type MArgs = ArgsOf<typeof name> | never
      type MData = DataOf<typeof name>

      // Allow the caller to e.g. handle mutation results
      queryClient.setMutationDefaults(baseKey, mutationOptions)

      type EndpointUseMutationOptions = UseMutationOptions<MData, unknown, MArgs>

      // useMutation(): wrap React-Query's useMutation
      const useMut = (
        options?: APIUseMutationOptions<typeof name>,
        ...args: MArgs
      ): UseMutationResult<typeof name> => {

        const mutationOptions: EndpointUseMutationOptions = {...options}
        // Do not allow overriding the events - they can be specified in the
        // endpoint definition or at the mutation call. The typescript type prohibits
        // this but we should enforce it at runtime for JS or ignored TS errors in order
        // to avoid unexpected behavior.
        for (const key of ['onSuccess', 'onSettled', 'onError', 'onMutate'] as Partial<keyof EndpointUseMutationOptions>[]) {
          if (Object.hasOwn(mutationOptions, key)) {
            delete mutationOptions[key]
          }
        }
        const useMutationResult = useMutation<MData, unknown, MArgs>(
          {
            mutationKey: [...baseKey, ...args],
            mutationFn: (variables) => mutation(...variables),
            ...mutationOptions,
          },
          queryClient,
        )

        return {
          ...useMutationResult,
          [name]: useMutationResult.mutate,
        } as UseMutationResult<typeof name>
      }

      const directMutateFn = (args: MArgs, options?: MutateOptions<MData, unknown, MArgs, unknown>) => {
        const observer = new MutationObserver<MData, unknown, MArgs, unknown>(queryClient, {
          ...options,
          mutationKey: [...baseKey, ...args ?? []],
          mutationFn: (variables) => mutation(...variables),
        })

        return observer.mutate(args ?? [])
      }

      ;(endpoints as any)[name] = {
        mutate: directMutateFn,
        useMutation: useMut,
      }
    }
  })

  // Events
  // We expect `config.events` to be something like:
  // {
  //   onSomethingHappened: event<[], { some: 'data' }>(),
  //   onConversationDeleted: event<[conversationId: string], boolean>(),
  //   onUserRenamed:       event<[userId: number], { id:number; newName: string }>(),
  // }
  //
  // Where the user can subscribe to an argument-less event,
  // e.g. `onSomethingHappened`, or an event with key arguments,
  // e.g. `onConversationDeleted` which takes a `conversationId` as the key argument.
  //
  // That is different from the payload, which is the data that is sent
  // when the event is emitted.
  // e.g.
  // useOnConversationDeleted(conversationId, (result) => {
  //   console.log(`Conversation ${conversationId} was deleted: ${result}`)
  // })
  //
  // -or-
  //
  // useOnSomethingHappened((data) => {})
  //
  //
  type EvAll = keyof NonNullable<typeof config.events>
  type EvDefs = NonNullable<typeof config.events>

  type KeyArgsOf<K extends EvAll> =
    EvDefs[K] extends EventDef<infer A, any> ? A : never
  type PayloadOf<K extends EvAll> =
    EvDefs[K] extends EventDef<any, infer P extends any[]> ? P : never

  function emitEvent<K extends EvAll>(
    eventName: K,
    ...argsAndPayload: [...KeyArgsOf<K>, PayloadOf<K>]
  ) {
    const payload = argsAndPayload[argsAndPayload.length - 1] as PayloadOf<K>
    const keyArgs = argsAndPayload.slice(
      0,
      argsAndPayload.length - 1,
    ) as KeyArgsOf<K>

    queryClient.setQueryData<PayloadOf<K>>(
      [...([config.key || '', eventName] as const), ...keyArgs] as [
        string,
        K,
        ...KeyArgsOf<K>,
      ],
      payload,
    )
  }

  type EventHooks = {
    /**
     * React hook to wrap useEffect with a subscription and unsubscription
     * to the event, firing the provided handler, and resubscribing
     * whenever the provided dependencies change.
     */
    [K in EvAll as `use${Capitalize<string & K>}`]: (
      handler?: (...result: PayloadOf<K>) => void,
      deps?: React.DependencyList,
      ...keyArgs: KeyArgsOf<K>
    ) => void
  } & {
    /**
     * React hook to get the latest payload for the event. Useful for one-off
     * events without needing to add extra state to your UI component
     */
    [K in EvAll as `useCurrent${Capitalize<string & K>}`]: (
      ...keyArgs: KeyArgsOf<K>
    ) => {
      hasEmitted: boolean
      /**
       * The latest data for the event, or undefined if no data has been set
       */
      data: PayloadOf<K> | undefined
    }
  } & {
    /**
     * Subscribe and unsubscribe to the event
     */
    [K in EvAll as `subscribeTo${Capitalize<string & K>}`]: (
      handler: (...result: PayloadOf<K>) => void,
      ...keyArgs: KeyArgsOf<K>
    ) => () => void
  } & {
    /**
     * Clear event data
     */
    [K in EvAll as `reset${Capitalize<string & K>}`]: (
      ...keyArgs: KeyArgsOf<K>
    ) => void
  }

  // Events implementation

  const eventHooks = {} as EventHooks

  ;(Object.keys(config.events || {}) as Array<EvAll>).forEach((eventName) => {
    const cap = `${(eventName as string)[0].toUpperCase()}${(eventName as string).slice(1)}`
    const hookName = `use${cap}`
    const hookNameUseCurrent = `useCurrent${cap}`
    const subscribeName = `subscribeTo${cap}`
    const handledName = `reset${cap}`

    const keyBase = [config.key || '', eventName] as const

    // register the emitter for the event
    // @ts-expect-error we need to fix the type of argsAndPayload
    (config.events as any)[eventName].registerEmitter((...args: any[]) => emitEvent(eventName, args))

    // useCurrentMyEvent
    ;(eventHooks as any)[hookNameUseCurrent] = (...keyArgs: any[]) => {
      const hookData = useQuery<PayloadOf<typeof eventName>>(
        {
          queryKey: [...keyBase, ...keyArgs],
          enabled: false,
          queryFn: () =>
            Promise.reject(
              new Error(
                `${eventName as string} is an event, not a query and should not try to fetch data`,
              ),
            ),
        },
        queryClient,
      )

      return {
        hasEmitted: hookData.isPlaceholderData,
        data: hookData.data,
      }
    }

    // subscribeToMyEvent
    ;(eventHooks as any)[subscribeName] = (
      handler: (...result: PayloadOf<typeof eventName>) => {},
      ...keyArgs: any[]
    ) => {
      const observer = new QueryObserver<PayloadOf<typeof eventName>>(
        queryClient,
        {
          queryKey: [...keyBase, ...keyArgs],
          enabled: false,
          queryFn: () =>
            Promise.reject(
              new Error(
                `${eventName as string} is an event, not a query and should not try to fetch data`,
              ),
            ),
        },
      )

      const unsubscribe = observer.subscribe((result) => {
        if (result.data !== undefined) {
          handler(...result.data)
        } else {
          // We would only get here if there is never any payload type for
          // this event.
          // @ts-expect-error no args
          handler()
        }
      })

      return unsubscribe
    }

    // useMyEvent
    ;(eventHooks as any)[hookName] = (
      handler: (...result: PayloadOf<typeof eventName>) => {},
      deps: React.DependencyList,
      ...keyArgs: any[]
    ) => {
      // Every time this function is called, we're probably going to get a new instance
      // of the handler, but we don't want to depend on it so that we re-subscribe
      // every time the handler changes. We also don't want to call an old version
      // of the handler when the event fires. Storing in a ref solves both requirements.
      const handlerRef = React.useRef(handler)
      handlerRef.current = handler
      React.useEffect(() => {
        const unsubscribe = (eventHooks as any)[subscribeName](
          handlerRef.current,
          ...keyArgs,
        )
        return () => {
          unsubscribe()
        }
      }, deps)
    }

    // Now that we have the subscribe pattern, we don't really need to reset the data
    ;(eventHooks as any)[handledName] = (...keyArgs: any[]) => {
      return () => {
        // Could consider using resetQueries here
        queryClient.resetQueries({
          queryKey: [...keyBase, ...keyArgs],
          exact: true,
        })
      }
    }
  })

  type MutationKeys = {
    [K in keyof RawEndpoints]: RawEndpoints[K] extends MutationEndpointDefinition<any, any> ? K : never
  }[keyof RawEndpoints] & string

  type QueryKeys = {
    [K in keyof RawEndpoints]: RawEndpoints[K] extends QueryEndpointDefinition<any, any> ? K : never
  }[keyof RawEndpoints] & string

  // Build convenience root access to endpoints
  type RootEndpointMethods =
  // Queries
  {
    // Named query React hook
    [K in QueryKeys as `use${Capitalize<string & K>}`]:
      (...args: ArgsOf<K>) => UseQueryResult<K>
  } & {
    // Direct access to endpoint to get non-hook current data, invalidate the query, etc.
    [K in QueryKeys]: APIEndpoints[K]
  } & {
      // Queries with placeholder data always have data, so we can have a
      // named data convenience hook if components don't need progress state.
      [K in ValidKey as RawEndpoints[K] extends PlaceholderQueryEndpointDefinition<
        any,
        any
      >
        ? `use${Capitalize<string & K>}Data`
        : never]: (...args: ArgsOf<K>) => DataOf<K>
  } &
  // Mutations
  {
    // useDoSomething: endpoints.doSomething.useMutation
    [K in MutationKeys as `use${Capitalize<string & K>}`]:
      (options?: APIUseMutationOptions<K>) => UseMutationResult<K>
  } & {
    // doSomething: endpoints.doSomething.mutate
    [K in MutationKeys]: Extract<APIEndpoints[K], { mutate: any }>['mutate']
  }

  const rootEndpointProperties = {} as RootEndpointMethods

  ;(Object.keys(config.endpoints) as Array<keyof RawEndpoints>).forEach(
    (name) => {
      const capitalized = `${(name as string)[0].toUpperCase()}${(name as string).slice(1)}`
      const hookName =
        `use${capitalized}` as `use${Capitalize<string & typeof name>}`

      if ('query' in config.endpoints[name]) {
        // @ts-expect-error: generated hook name
        rootEndpointProperties[hookName] = (endpoints as any)[name].useQuery

        // @ts-expect-error
        rootEndpointProperties[name] = (endpoints as any)[name]

        if ('placeholderData' in config.endpoints[name]) {
          // @ts-expect-error: generated hook name
          rootEndpointProperties[`use${capitalized}Data`] = (...args: any[]) => {
            return (endpoints as any)[name].useQuery(...args).data
          }
        }
      }

      if ('mutation' in config.endpoints[name]) {
        // @ts-expect-error: we know that `endpoints[name].useMutation` matches the signature
        rootEndpointProperties[hookName] = (endpoints as any)[name].useMutation
        // @ts-expect-error
        rootEndpointProperties[name] = (endpoints as any)[name].mutate
      }
    },
  )

  const actions = config.actions as any as APIActions

  // @ts-expect-error
  window.queryClient = queryClient

  // Convenient access to query endpoint child functions

  const api = {
    endpoints,
    emitEvent,
    actions,
    ...rootEndpointProperties,
    ...eventHooks,
    close: () => {
      queryClient.cancelQueries()
      queryClient.clear()
      queryClient.unmount()
    },
  }

  // @ts-expect-error
  window.api = api

  return api
}
