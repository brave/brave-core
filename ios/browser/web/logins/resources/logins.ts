// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendWebKitMessage,
  sendWebKitMessageWithReply,
} from '//ios/web/public/js_messaging/resources/utils.js';

const HANDLER_NAME = 'LoginsMessageHandler';

const KEYCODE_ARROW_DOWN = 40;

let gEnabled = true;
let gStoreWhenAutocompleteOff = true;
let gAutofillForms = true;

interface LoginData {
  hostname: string;
  formSubmitURL: string;
  httpRealm: string;
  username: string;
  password: string;
  usernameField: string;
  passwordField: string;
}

function isUsernameFieldType(element: Element): boolean {
  if (!(element instanceof HTMLInputElement)) {
    return false;
  }
  const fieldType = element.hasAttribute('type') ?
      element.getAttribute('type')!.toLowerCase() :
      element.type;
  return fieldType === 'text' || fieldType === 'email' ||
      fieldType === 'url' || fieldType === 'tel' ||
      fieldType === 'number';
}

function isAutocompleteDisabled(element: Element | null): boolean {
  if (element && element.hasAttribute('autocomplete') &&
      element.getAttribute('autocomplete')!.toLowerCase() === 'off') {
    return true;
  }
  return false;
}

interface PasswordFieldEntry {
  index: number;
  element: HTMLInputElement;
}

function getPasswordFields(
    form: HTMLFormElement,
    skipEmpty: boolean): PasswordFieldEntry[] | null {
  const pwFields: PasswordFieldEntry[] = [];
  for (let i = 0; i < form.elements.length; i++) {
    const element = form.elements[i];
    if (!(element instanceof HTMLInputElement) ||
        element.type !== 'password') {
      continue;
    }
    if (skipEmpty && !element.value) {
      continue;
    }
    pwFields.push({index: i, element});
  }
  if (pwFields.length === 0 || pwFields.length > 3) {
    return null;
  }
  return pwFields;
}

function getFormFields(
    form: HTMLFormElement,
    isSubmission: boolean): [HTMLInputElement | null, HTMLInputElement | null,
                             HTMLInputElement | null] {
  const pwFields = getPasswordFields(form, isSubmission);
  if (!pwFields) {
    return [null, null, null];
  }

  // getPasswordFields guarantees at least one entry; destructure for safe
  // access without repeated non-null assertions below.
  const [pw0, pw1, pw2] = pwFields as [
    PasswordFieldEntry,
    PasswordFieldEntry | undefined,
    PasswordFieldEntry | undefined,
  ];

  let usernameField: HTMLInputElement | null = null;
  for (let i = pw0.index - 1; i >= 0; i--) {
    const element = form.elements[i];
    if (isUsernameFieldType(element as Element)) {
      usernameField = element as HTMLInputElement;
      break;
    }
  }

  if (!isSubmission || pwFields.length === 1) {
    return [usernameField, pw0.element, null];
  }

  const val0 = pw0.element.value;
  const val1 = pw1!.element.value;
  const val2 = pw2 ? pw2.element.value : null;

  let oldPasswordField: HTMLInputElement | null;
  let newPasswordField: HTMLInputElement;

  if (pwFields.length === 3) {
    if (val0 === val1 && val1 === val2) {
      newPasswordField = pw0.element;
      oldPasswordField = null;
    } else if (val0 === val1) {
      newPasswordField = pw0.element;
      oldPasswordField = pw2!.element;
    } else if (val1 === val2) {
      oldPasswordField = pw0.element;
      newPasswordField = pw2!.element;
    } else if (val0 === val2) {
      newPasswordField = pw0.element;
      oldPasswordField = pw1!.element;
    } else {
      return [null, null, null];
    }
  } else {
    if (val0 === val1) {
      newPasswordField = pw0.element;
      oldPasswordField = null;
    } else {
      oldPasswordField = pw0.element;
      newPasswordField = pw1!.element;
    }
  }

  return [usernameField, newPasswordField, oldPasswordField];
}

function dispatchKeyboardEvent(
    element: HTMLElement, eventName: string, keyCode: number): void {
  element.dispatchEvent(
      new KeyboardEvent(eventName, {bubbles: true, cancelable: true, keyCode}));
}

function fillForm(
    form: HTMLFormElement,
    autofillForm: boolean,
    ignoreAutocomplete: boolean,
    clobberPassword: boolean,
    userTriggered: boolean,
    foundLogins: LoginData[]): [boolean, LoginData[]] {
  const [usernameField, passwordField] = getFormFields(form, false);

  if (!passwordField) {
    return [false, foundLogins];
  }
  if (passwordField.disabled || passwordField.readOnly) {
    return [false, foundLogins];
  }

  let maxUsernameLen = Number.MAX_VALUE;
  let maxPasswordLen = Number.MAX_VALUE;
  if (usernameField && usernameField.maxLength >= 0) {
    maxUsernameLen = usernameField.maxLength;
  }
  if (passwordField.maxLength >= 0) {
    maxPasswordLen = passwordField.maxLength;
  }

  const logins = foundLogins.filter(
      l => l.username.length <= maxUsernameLen &&
          l.password.length <= maxPasswordLen);

  if (logins.length === 0) {
    return [false, foundLogins];
  }

  // Don't clobber an existing password.
  if (passwordField.value && !clobberPassword) {
    return [false, foundLogins];
  }

  let selectedLogin: LoginData | null = null;

  if (usernameField &&
      (usernameField.value || usernameField.disabled ||
       usernameField.readOnly)) {
    const username = usernameField.value.toLowerCase();
    const matchingLogins =
        logins.filter(l => l.username.toLowerCase() === username);
    if (matchingLogins.length) {
      for (const l of matchingLogins) {
        if (l.username === usernameField.value) {
          selectedLogin = l;
          break;
        }
      }
      if (!selectedLogin) {
        selectedLogin = matchingLogins[0] ?? null;
      }
    }
  } else if (logins.length === 1) {
    selectedLogin = logins[0] ?? null;
  } else {
    const matchingLogins = usernameField ?
        logins.filter(l => l.username) :
        logins.filter(l => !l.username);
    selectedLogin = matchingLogins[0] ?? null;
  }

  let isFormDisabled = false;
  if (!ignoreAutocomplete &&
      (isAutocompleteDisabled(form) ||
       isAutocompleteDisabled(usernameField) ||
       isAutocompleteDisabled(passwordField))) {
    isFormDisabled = true;
  }

  if (selectedLogin && autofillForm && !isFormDisabled) {
    if (usernameField) {
      const disabledOrReadOnly =
          usernameField.disabled || usernameField.readOnly;
      const userNameDiffers =
          selectedLogin.username !== usernameField.value;
      const userEnteredDifferentCase = userTriggered && userNameDiffers &&
          usernameField.value.toLowerCase() ===
              selectedLogin.username.toLowerCase();

      if (!disabledOrReadOnly && !userEnteredDifferentCase &&
          userNameDiffers) {
        usernameField.value = selectedLogin.username;
        if (document.activeElement !== usernameField) {
          usernameField.dispatchEvent(new Event('change'));
        }
        dispatchKeyboardEvent(usernameField, 'keydown', KEYCODE_ARROW_DOWN);
        dispatchKeyboardEvent(usernameField, 'keyup', KEYCODE_ARROW_DOWN);
      }
    }
    if (passwordField.value !== selectedLogin.password) {
      passwordField.value = selectedLogin.password;
      if (document.activeElement !== passwordField) {
        passwordField.dispatchEvent(new Event('change'));
      }
      dispatchKeyboardEvent(passwordField, 'keydown', KEYCODE_ARROW_DOWN);
      dispatchKeyboardEvent(passwordField, 'keyup', KEYCODE_ARROW_DOWN);
    }
    return [true, foundLogins];
  }

  return [false, foundLogins];
}

async function asyncFindLogins(
    form: HTMLFormElement): Promise<LoginData[]> {
  const fields = getFormFields(form, false);
  if (!fields[0] || !fields[1]) {
    return [];
  }

  fields[0].addEventListener('blur', onBlur);

  const formOrigin = document.documentURI;
  const actionOrigin = form.action || form.baseURI;
  if (!actionOrigin) {
    return [];
  }

  try {
    const logins = await sendWebKitMessageWithReply(HANDLER_NAME, {
      type: 'request',
      formOrigin,
      actionOrigin,
    });
    return Array.isArray(logins) ? (logins as LoginData[]) : [];
  } catch (_) {
    return [];
  }
}

async function onBlur(event: Event): Promise<void> {
  if (!gEnabled) {
    return;
  }

  const acInputField = event.target as HTMLInputElement;
  if (!(acInputField.ownerDocument instanceof HTMLDocument)) {
    return;
  }
  if (!isUsernameFieldType(acInputField)) {
    return;
  }

  const acForm = acInputField.form;
  if (!acForm) {
    return;
  }
  if (!acInputField.value) {
    return;
  }

  const [usernameField, passwordField] = getFormFields(acForm, false);
  if (usernameField === acInputField && passwordField) {
    const logins = await asyncFindLogins(acForm);
    fillForm(acForm, true, true, true, true, logins);
  }
}

function onFormSubmit(form: HTMLFormElement): void {
  if (!gEnabled) {
    return;
  }

  const hostname = document.documentURI;
  if (!hostname) {
    return;
  }

  const formSubmitURL = form.action || form.baseURI;
  const fields = getFormFields(form, true);
  const [usernameField, newPasswordField, oldPasswordField] = fields;

  if (!newPasswordField) {
    return;
  }

  if ((isAutocompleteDisabled(form) ||
       isAutocompleteDisabled(usernameField) ||
       isAutocompleteDisabled(newPasswordField) ||
       isAutocompleteDisabled(oldPasswordField)) &&
      !gStoreWhenAutocompleteOff) {
    return;
  }

  sendWebKitMessage(HANDLER_NAME, {
    type: 'submit',
    data: JSON.stringify({
      hostname,
      username: usernameField ? usernameField.value : '',
      usernameField: usernameField ? usernameField.name : '',
      password: newPasswordField.value,
      passwordField: newPasswordField.name,
      formSubmitURL,
    }),
  });
}

async function findLogins(form: HTMLFormElement): Promise<void> {
  try {
    const logins = await asyncFindLogins(form);
    if (logins.length > 0) {
      fillForm(form, gAutofillForms, false, false, false, logins);
    }
  } catch (_) {
    // Eat errors to avoid leaking them to the page
  }
}

function findForms(nodes: NodeList): void {
  for (let i = 0; i < nodes.length; i++) {
    const node = nodes[i];
    if (!node) {
      continue;
    }
    if ((node as Element).nodeName === 'FORM') {
      findLogins(node as HTMLFormElement);
    } else if (node.hasChildNodes()) {
      findForms(node.childNodes);
    }
  }
}

const observer = new MutationObserver(mutations => {
  for (const mutation of mutations) {
    findForms(mutation.addedNodes);
  }
});

window.addEventListener('load', () => {
  observer.observe(document.body, {
    attributes: false,
    childList: true,
    characterData: false,
    subtree: true,
  });
  for (let i = 0; i < document.forms.length; i++) {
    const form = document.forms[i];
    if (form) {
      findLogins(form);
    }
  }
});

window.addEventListener('submit', event => {
  try {
    for (let i = 0; i < document.forms.length; i++) {
      const form = document.forms[i];
      if (form) {
        findLogins(form);
      }
    }
    onFormSubmit(event.target as HTMLFormElement);
  } catch (_) {
    // Eat errors to avoid leaking them to the page
  }
});

window.addEventListener('pagehide', event => {
  if ((event as PageTransitionEvent).persisted) {
    return;
  }

  const isSubmittedForm = (form: HTMLFormElement): boolean => {
    const fields = getFormFields(form, false);
    if (!fields[0] || !fields[1]) {
      return false;
    }

    const actionOrigin = form.action || form.baseURI;
    if (!actionOrigin) {
      return false;
    }

    for (const field of fields) {
      if (field && (!field.value || field.value.length === 0)) {
        return false;
      }
    }
    return true;
  };

  for (const form of document.forms) {
    if (isSubmittedForm(form)) {
      try {
        onFormSubmit(form);
      } catch (_) {
        // Eat errors to avoid leaking them to the page
      }
    }
  }
});
