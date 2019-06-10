import * as runtimeActions from '../../constants/runtimeActionTypes'
import {Actions} from '../../types/actions'
import * as browserActionAPI from '../api/browserActionAPI'

type State = {}

export default function runtimeReducer(state: State = {}, action: Actions):
    State {
      switch (action.type) {
        case runtimeActions.RUNTIME_DID_STARTUP: {
          browserActionAPI.init()
          break
        }
      }
      return state
    }
