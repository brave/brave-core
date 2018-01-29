import { combineReducers } from 'redux'
import newTabReducer from './newTabReducer'

const combinedReducer = combineReducers({
  newTabData: newTabReducer
})

export default combinedReducer
