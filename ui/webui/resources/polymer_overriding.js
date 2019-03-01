// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// This is in global scope,
// so create a temporary scope.
(function () {

  // rewrite dom-module IDs so we can replace with ours
  // const oldRegisterFn = window.Polymer.DomModule.prototype.register
  // function newRegisterFn(id) {
  //   if (id === 'br-toolbar')
  //     id == 'br-toolbar-test'
  // }

  window.BravePatching = window.BravePatching || {}
  const oldFn = window.Polymer._polymerFn
  function newPolymerFn(component){
    if (component && component.is) {
      AddBraveBehaviors(component)
    }
    const polymerComponent = oldFn(component)
    if (component && component.is === 'cr-toolbar') {
      const domModule = Polymer.DomModule.import('cr-toolbar')
      const template = domModule.querySelector('template')
      console.log(domModule, template)
      const testToolbar = template.content.querySelector('div')
      console.log('br-toolbar', testToolbar)
      // const testToolbar = polymerComponent.template.content.querySelector('.br-toolbar')
      // console.log('cr-toolbar register', testToolbar.outerHTML)
      const testEl = document.createElement('h2')
      testEl.innerText = '[[settingsTitle]]'
      testToolbar.appendChild(testEl)
      console.log('cr-toolbar modify', testToolbar.outerHTML)
    }
    return polymerComponent
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
