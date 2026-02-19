// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const curUrl = window.location.href
console.log("[PSST POLICY SCRIPT] Current URL: " + curUrl);
// Timeout to wait of the URL opening
const WAIT_FOR_PAGE_TIMEOUT = 6000

// Use tasks as list of the policy settings tasks to apply
const PSST_TASKS = params.tasks
const PSST_TASKS_LENGTH = params.tasks?.length ?? 0

// Flag which is present only for the first (initial) execution of the policy script
const PSST_INITIAL_EXECUTION_FLAG = params.initial_execution ?? false

const PSST_LOCALSTORAGE_KEY = 'psst'

// State of operations
const psstState = {
  STARTED: "started",
  COMPLETED: "completed"
}

/* Helper functions */
const goToUrl = url => {
console.log("[PSST] goToUrl: " + url)
  window.location.href = url
}

const checkCheckboxes = (turnOff) => {
  const checkboxes = document.querySelectorAll("input[type='checkbox']")
  if (checkboxes.length === 1) {
    if (turnOff) {
      if (checkboxes[0].checked) {
        // Uncheck it
        checkboxes[0].click()
      }
    }
    return { success: true }
  } else {
    return { success: false, error: 'No checkbox found' }
  }
}

const waitForCheckboxToLoadWithTimeout = (timeout, turnOff) => {
  const startTime = Date.now()
  let result = null
  
  while ((Date.now() - startTime) < timeout) {
    result = checkCheckboxes(turnOff)
    if (result.success) {
      console.log("[PSST] Set Checkbox state successfully")
      return result
    }
    const now = Date.now()
    while (Date.now() - now < 300) { /* busy wait 100ms */ }
  }
  
  // Timeout reached, try one more time
  result = checkCheckboxes(turnOff)
  if (!result.success) {
    result.error = 'Timeout waiting for checkbox'
  }
  return result
}

const getAvailableTasks = (psst) => {
  const tasksInList = (psst?.tasks_list?.length ?? 0)
  console.log('[PSST] getAvailableTasks tl:' + (psst?.tasks_list?.length ?? 0) + ' ct:' + ((psst?.current_task ?? null) === null ? 0 : 1))
  return tasksInList + ((psst?.current_task ?? null) === null ? 0 : 1)
}

const getProcessedTasks = (psst) => {
    return (psst?.applied_tasks?.length ?? 0) + (psst?.errors?.length ?? 0)
}

const calculateProgress = (processedTasks, availableTasks) => {
  console.log(`[PSST] calculateProgress processedTasks:${processedTasks} availableTasks:${availableTasks}`)
  
  const processed = Number(processedTasks) || 0
  const available = Number(availableTasks) || 0
  const total = processed + available
  
  return total === 0 ? 0 : (processed / total) * 100
}

const getResult = (result, psst) => {
  const result_value = {
    result: result,
    psst: psst
  };
  console.log("[PSST POLICY SCRIPT] Result:", JSON.stringify(result_value));
   return result_value;
}

const start = () => {
  const curUrl = window.location.href;
  console.log(`[PSST] start #100 tasks:`, PSST_TASKS ?? []);

  // Ensure we have an array and safely get the first task (if any)
  const tasks = Array.isArray(PSST_TASKS) ? [...PSST_TASKS] : [];
  const next_task = tasks.shift() || null;

  const psst = {
    state: psstState.STARTED,
    tasks_list: tasks,
    start_url: curUrl,
    progress: calculateProgress(0, PSST_TASKS_LENGTH),
    current_task: next_task,
    applied_tasks: []
  };

  return [psst, next_task?.url ?? null];
};

const saveAndGoToNextUrl = (psst, nextUrl) => {
  // Save the psst object to local storage.
  window.parent.localStorage.setItem(PSST_LOCALSTORAGE_KEY, JSON.stringify(psst))
  // Go to the next URL.
  goToUrl(nextUrl)
}

(() => {
  const psst = window.parent.localStorage.getItem(PSST_LOCALSTORAGE_KEY)
  console.log("[PSST] #100 PSST_INITIAL_EXECUTION_FLAG:", PSST_INITIAL_EXECUTION_FLAG)
  console.log("[PSST] #101 psst:", psst)
  
  if (!psst || PSST_INITIAL_EXECUTION_FLAG) {
    // Start applying-policy
    const [psstObj, nextUrl] = start()
    saveAndGoToNextUrl(psstObj, nextUrl)
    return getResult(false, psstObj)
  }

  const psstObj = JSON.parse(psst)
  if (!psstObj) {
    return getResult(false, null)
  }

  if (psstObj.state === psstState.COMPLETED) {
    return getResult(true, psstObj)
  }

  const checkboxResult = waitForCheckboxToLoadWithTimeout(
    WAIT_FOR_PAGE_TIMEOUT,
    true /* turnOff */
  )
  
  const current_task = psstObj.current_task
  if(current_task) {
    psstObj.applied_tasks.push(checkboxResult.success ? psstObj.current_task : {
        url: current_task.url,
      description: current_task.description,
      error_description: checkboxResult.error
    })
  }

  const next_task = psstObj.tasks_list.shift()
  let nextUrl = null
  if (!next_task) {
    psstObj.state = psstState.COMPLETED
    nextUrl = psstObj.start_url
  } else {
    nextUrl = next_task.url
  }

  psstObj.current_task = next_task
  psstObj.progress = calculateProgress(getProcessedTasks(psstObj), getAvailableTasks(psstObj))

  saveAndGoToNextUrl(psstObj, nextUrl)
  return getResult(false, psstObj)
})()
