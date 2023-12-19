import { RegisterStyleOverride, RegisterPolymerPrototypeModification, RegisterPolymerTemplateModifications } from 'chrome://resources/brave/polymer_overriding.js'
import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import 'chrome://resources/brave/leo.bundle.js'

RegisterStyleOverride('settings-toggle-button', html`
  <style>
  </style>
`)

RegisterPolymerTemplateModifications({
  'settings-toggle-button': (template: HTMLTemplateElement) => {
    template.querySelector('cr-toggle')
      ?.replaceWith(html`<leo-toggle
        id="control"
        size="small"
        checked="{{checked}}"
        on-change="nalaChange_"
        disabled="[[controlDisabled(disabled, pref)]]">
      </leo-toggle>`.content)
  }
})

RegisterPolymerPrototypeModification({
  'settings-toggle-button': prototype => {
    // We wrap the onChange_ function so we can emit the same event as the CrToggle
    prototype.nalaChange_ = function(e: CustomEvent<{ checked: boolean }>) {
      prototype.onChange_.apply(this, [new CustomEvent(e.type, { detail: e.detail.checked })])
    }
  }
})
