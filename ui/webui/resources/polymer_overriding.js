// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// This is in global scope,
// so create a temporary scope.
(function () {
  window.BravePatching = window.BravePatching || {}
  const oldFn = window.Polymer._polymerFn
  function newPolymerFn(component){
    if (component && component.is) {
      AddBraveBehaviors(component)
    }
    return oldFn(component)
  }
  window.Polymer._polymerFn = newPolymerFn

  const allBehaviorsMap = {}

  function AddBraveBehaviors(component) {
    if (allBehaviorsMap[component.is]) {
      component.behaviors = component.behaviors || []
      component.behaviors.push(...allBehaviorsMap[component.is])
    }
  }

  function BraveRegisterComponentBehaviors(behaviorsMap) {
    Object.assign(allBehaviorsMap, behaviorsMap)
  }

  // Accessible to other modules
  window.BravePatching.registerPolymerBehaviors = BraveRegisterComponentBehaviors
})()
