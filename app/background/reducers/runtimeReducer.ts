import { Actions } from '../../types/actions'
import * as runtimeActions from '../../constants/runtimeActionTypes'
import * as browserActionAPI from '../api/browserActionAPI'

type State = { }

export default function runtimeReducer (state: State = { }, action: Actions): State {
  switch (action.type) {
    case runtimeActions.RUNTIME_DID_STARTUP: {
      browserActionAPI.init()
      break
    }
  }
  return state
}
