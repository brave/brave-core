// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

// Global overrides
import CrButtonStyleTemplate from './overrides/cr_button.js'
import CrToggleStyleTemplate from './overrides/cr_toggle.js'

const debug = false

if (debug) {
  // Useful to diagnose module definition timing for template modification
  // debugging.
  console.log("MODULE: polymer_overriding")
}

const allBehaviorsMap = {}
const allPropertiesMap = {}
const componentPropertyModifications = {}

function addBraveBehaviors(moduleName, component) {
  if (allBehaviorsMap[moduleName]) {
    component.behaviors = component.behaviors || []
    component.behaviors.push(...allBehaviorsMap[moduleName])
    delete allBehaviorsMap[moduleName]
  }
}

function addBraveProperties(moduleName, component) {
  if (allPropertiesMap[moduleName]) {
    component.properties = component.properties || {}
    component.properties = {
      ...component.properties,
      ...allPropertiesMap[moduleName]
    }
    delete allPropertiesMap[moduleName]
  } else {
    componentPropertyModifications[moduleName] = component
  }
}

const allBraveTemplateModificationsMap = {}

function addBraveTemplateModifications(moduleName, component, modifyFn) {
  const template = component.template || component._template
  if (template) {
    const templateContent = template.content
    const t0 = debug && performance.now()
    modifyFn(templateContent)
    const t1 = debug && performance.now()
    if (debug)
      console.debug(`Modifying template '${moduleName}' took ${t1 - t0}ms`)
  } else {
    console.error(`Source template not found for override of component "${moduleName}"`)
  }
}

const styleOverridePrefix = 'brave-override-style-'

function addBraveStyleOverride(moduleName, component, template = component.template || component._template) {
  if (!template) {
    console.error(`No template found for component (${moduleName}) with found style overrides`, component)
    return
  }
  const styleElement = template.content.querySelector('style')
  if (!styleElement) {
    console.error(`No style element found for component (${moduleName}) with found style overrides`, component)
    return
  }
  const overrideModuleName = styleOverridePrefix + moduleName
  styleElement.setAttribute(
    'include',
    `${styleElement.getAttribute('include')} ${overrideModuleName}`
  )
  if (debug)
    console.log(`Brave Style Override added for ${moduleName}`, styleElement)
}

export function RegisterPolymerComponentBehaviors(behaviorsMap) {
  if (debug) {
    console.log('RegisterPolymerComponentBehaviors', ...Object.keys(behaviorsMap))
  }
  Object.assign(allBehaviorsMap, behaviorsMap)
}

export function RegisterPolymerComponentProperties(propertiesMap) {
  if (debug) {
    console.debug('RegisterPolymerComponentProperties', ...Object.keys(propertiesMap))
  }
  Object.assign(allPropertiesMap, propertiesMap)
  for (const componentName in propertiesMap) {
    if (componentPropertyModifications[componentName]) {
        addBraveProperties(componentName, componentPropertyModifications[componentName])
    }
  }
}

export function RegisterPolymerTemplateModifications(modificationsMap) {
  if (debug) {
    console.log('RegisterPolymerTemplateModifications', ...Object.keys(modificationsMap))
  }
  const awaitingComponentModifications = {}
  for (const componentName in modificationsMap) {
    const modifyFn = modificationsMap[componentName]
    const existingComponent = window.customElements.get(componentName)
    if (!existingComponent) {
      awaitingComponentModifications[componentName] = modificationsMap[componentName]
      continue
    }
    // Component is already defined, modify now.
    addBraveTemplateModifications(componentName, existingComponent, modifyFn)
  }
  Object.assign(allBraveTemplateModificationsMap, awaitingComponentModifications)
}

const moduleNamesWithStyleOverrides = []

export async function RegisterStyleOverride(componentName, styleTemplate) {
  // Inform polymer of styles by creating a dom-module.
  //   <dom-module id="${styleOverridePrefix + componentName}">
  //     <template>
  //       ${styleOuterHTML}
  //     </template>
  //   </dom-module>
  // This is the same way that polymer_modulizer uses for style-module types.
  const template = document.createElement('template')
  const domModule = document.createElement('dom-module')
  domModule.id = styleOverridePrefix + componentName
  template.appendChild(domModule)
  domModule.appendChild(styleTemplate)
  document.body.appendChild(template.content.cloneNode(true))
  if (debug) {
    console.log(`REGISTERING STYLE OVERRIDE for ${componentName}`, styleTemplate)
  }
  // If module has already been defined, it's not ideal as the unmodified
  // template may already have been cloned for an element. However, let's make
  // an attempt and apply it anyway. Otherwise, we wait until it's defined and
  // then modify the template as soon as possible.
  const existingComponent = window.customElements.get(componentName)
  if (existingComponent) {
    addBraveStyleOverride(componentName, existingComponent)
  } else {
    // Cannot await CustomElementRegistry.whenDefined here
    // since getting in the async queue will mean this template
    // mofification happens too late. Instead, save this in a list
    // so that the template modification can happen inside the
    // customElements.define hook.
    moduleNamesWithStyleOverrides.push(componentName)
  }
}

export function OverrideIronIcons(iconSetName, overridingIconSetName, iconOverrides) {
  // Here, we change the underlying DOM.
  // We cannot simply change `IconSet._icons` since
  // there are occasions where this will get re-parsed from DOM elements.
  // const meta = Base.create('iron-meta', {type: 'iconset'})
  const meta = document.createElement('iron-meta')
  meta.type = 'iconset'
  const srcIconSet = meta.byKey(iconSetName)
  if (!srcIconSet) {
    console.error(`OverrideIronIcons: source icon set "${iconSetName} not found.`)
    return
  }
  const overrideIconSet = meta.byKey(overridingIconSetName)
  if (!overrideIconSet) {
    console.error(`OverrideIronIcons: overriding icon set "${overridingIconSetName} not found.`)
    return
  }
  for (const chromiumIconName in iconOverrides) {
    const chromiumIcon = srcIconSet.querySelector(`#${chromiumIconName}`)
    if (!chromiumIcon) {
      console.error(`[brave overrides] Could not find chromium icon '${chromiumIconName}' in iconset '${iconSetName}' for replacement!`)
      continue
    }
    const braveIconName = iconOverrides[chromiumIconName]
    const braveIcon = overrideIconSet.querySelector(`#${braveIconName}`)
    if (!braveIcon) {
      console.error(`[brave overrides] Could not find brave icon '${braveIconName}' in iconset '${overridingIconSetName}' for replacement!`)
      continue
    }
    // replace
    while (chromiumIcon.firstChild) {
      chromiumIcon.firstChild.remove()
    }
    while (braveIcon.firstChild) {
      chromiumIcon.appendChild(braveIcon.firstChild)
    }
  }
  // Ensure icons get re-parsed if already parseds
  // `getIconNames` ensures this._icons in iron-iconset-svg is re-parsed
  // from DOM. If that changes, we'll need to find another way,
  // perhaps `srcIconSet._icons = srcIconSet._createIconMap()`
  srcIconSet.getIconNames()
}

// Overriding Polymer.Class only works for some
// chromium components which call Polymer() and pass objects rather
// than classes.
const oldClass = Polymer.Class
Polymer.Class = function (info, mixin) {
  if (!info) {
    console.warn('Polymer.Class requires `info` argument');
  }
  const name = info.is
  if (!name) {
    if (debug) {
      console.warn(`Non-polymer-like polymer class`, info)
    }
    return oldClass(info, mixin)
  }
  if (debug) {
    console.debug(`Polymer component legacy registering: ${name}`, info)
  }
  addBraveBehaviors(name, info)
  return oldClass(info, mixin)
}

// Also override for components which do not call Polymer() but instead
// inherit from PolymerElement.
const oldPrepareTemplate = PolymerElement._prepareTemplate;
PolymerElement._prepareTemplate = function BravePolymer_PrepareTemplate() {
  oldPrepareTemplate.call(this)
  const name = this.is
  if (!name) {
    if (debug) {
      console.warn('PolymerElement defined with no name', this, this.prototype)
    }
    return
  }
  if (debug) {
    console.log('PolymerElement defined: ', name, this, this.prototype)
  }
  // Perform modifications
  // TODO(petemill): support mixins (behaviors). Older style Behaviors won't
  // work as they are a legacy polymer feature, now migrated to subclassing.
  addBraveProperties(name, this.prototype)
  const templateModifyFn = allBraveTemplateModificationsMap[name]
  if (templateModifyFn) {
    addBraveTemplateModifications(name, this.prototype, templateModifyFn)
    delete allBraveTemplateModificationsMap[name]
  }
  if (moduleNamesWithStyleOverrides.includes(name)) {
    addBraveStyleOverride(name, this.prototype)
  }
}

// Overrides for all pages
RegisterStyleOverride('cr-toggle', CrToggleStyleTemplate)
RegisterStyleOverride('cr-button', CrButtonStyleTemplate)
