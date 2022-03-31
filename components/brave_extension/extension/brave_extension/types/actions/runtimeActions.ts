import * as types from '../../constants/runtimeActionTypes'

interface RuntimeDidStartupReturn {
  type: typeof types.RUNTIME_DID_STARTUP
}

export type RuntimeDidStartup = () => RuntimeDidStartupReturn

export type runtimeActions = RuntimeDidStartupReturn
