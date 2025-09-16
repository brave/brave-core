# Privacy Settings Selection for Sites Tool (PSST)

The feature helps tweak privacy settings on popular websites.

## Spec
https://docs.google.com/document/d/1ccnBWBV_KkknZpZYxcOXTwtfIiaSzeLS5dMd08N2pbs/edit?tab=t.0

# PSST Common Workflow

1. The user opens any website.
2. PSST feature detects that it is supported website (it means that browser loads PSST CRX component and find the appropriate user and policy scripts).
3. The browser injects the user script into the page. The primary goal of the user script is to ensure that the user is signed in to the current website and return a list of URLs where we should apply the privacy settings.
    - the results of the user script execution we process here:</br> `PsstTabWebContentsObserver::OnUserScriptResult`
4. If the user is not signed in, we just break the flow.
5. If the user is signed in and we have a list of URLs, we will display a consent modal dialog. In this dialog, the user can choose their preferred privacy settings and track the progress of the operation.
6. When the user clicks the OK button, we inject the policy script and execute it.
7. The policy script does the next things: 
    - saves the list of URLs (tasks) to local storage to be available after the next navigation;
    - calculates current progress;
    - takes the first URL (URL_1) and marks it as current;
    - navigates to that URL;
    - returns the progress and applied tasks list to the back-end: `PsstTabWebContentsObserver::OnPolicyScriptResult(int nav_entry_id,base::Value script_result)`</br>
    where after deserialization and validation, we update the flow's status on the consent dialog, by calling the UI delegate's method:</br>
    `UpdateTasks(long progress,const std::vector<PolicyTask>& applied_tasks)`</br>
8. When navigation to URL_1 is complete, and it is a supported website, the user script is injected and executed. Then the same happens as in points #3,4.
9. Since the workflow is running and the consent dialog is visible, we inject the policy script.
10. Once injected, the policy script loads saved URLs (tasks) from local storage, takes the current' URL (see p. #7), and applies the privacy setting.
11. Once the privacy setting is applied, the policy script marks the current URL as applied, takes the next one from the available URL (task) list and marks it as current, saves all the info to local storage, and navigates to the new current one.
12. Then we enumerate each available URL in the list and do the same as in points #8-11.
13. When all tasks are completed, the user sees all status and progress information in the consent dialog.

### Workflow failure cases

1. <a id="script-times-out"></a>The script times out or page crashes.</br> This happens when an injected script (either from a user or a policy) never finishes running. Common causes include an infinite loop, a promise that never resolves, or renderer crash can lead to the same situation, when we need to stop the process and notify the front end.

#### Handling Script Timeouts and page crashes

The `base::OneShotTimer timeout_timer_;` is used to handle cases when a script fails to finish running, (i.e. ["The script times out or page crashes"](#script-times-out)).</br>
Use the next method for script execution as it starts the timer first (with interval: 15 seconds):
```
void PsstTabWebContentsObserver::RunWithTimeout(
    const int last_committed_entry_id,
    const std::string& script,
    InsertScriptInPageCallback callback) {
  timeout_timer_.Start(
      FROM_HERE, kScriptTimeout,
      base::BindOnce(&PsstTabWebContentsObserver::OnScriptTimeout,
                     weak_factory_.GetWeakPtr(), last_committed_entry_id));
  inject_script_callback_.Run(script, std::move(callback));
}

```
Parameters:</br>
`const int last_committed_entry_id` -  the unique last committed entry ID;</br>
`const std::string& script` - the script to be injected;</br>
`InsertScriptInPageCallback callback` - callback function that handles the script result once it executes;

The `PsstTabWebContentsObserver::OnScriptTimeout` is special timeout handler, which stops the other script execution result handlers by calling the `weak_factory_.InvalidateWeakPtrs();`

# PSST CRX Component

Contains set of rules and scripts for small number of very popular sites (Google, Facebook, Twitter, Twitch, etc.). 
Each rule set would be managed in open source (similar to https://github.com/brave/adblock-lists), and shipped daily to users.

Component ID: `lhhcaamjbmbijmjbnnodjaknblkiagon`
Component version: `1`

### Component's folder structure:

```
<component id>/<component version>/
 |_ manifest.json
 |_ psst.json
 |_ scripts/
    |_ twitter/
        |_ user.js
        |_ policy.js
    |_ linkedin/
        |_ user.js
        |_ policy.js
```

#### psst.json

Contains a set of rules for each supported website:

Example:
```
[
    {
        "name": "twitter",
        "include": [
            "https://x.com/*"
        ],
        "exclude": [
        ],
        "version": 4,
        "user_script": "user.js",
        "policy_script": "policy.js"
    },
    {
        "name": "linkedin",
        "include": [
            "https://www.linkedin.com/*"
        ],
        "exclude": [
        ],
        "version": 1,
        "user_script": "user.js",
        "policy_script": "policy.js"
    }
]
```

#### user.js

Script which helps to find the user identifier for the currently-logged-in user on the current website. We need this in order to apply PSST.
The output of user script execution is JSON, which contains the following fields:
`user` - contains the identifier of the logged-in user for the current website.
`tasks` - list of objects (url and description pairs) where `url` is the URL of the settings page for the website that we propose to change and its description.

Example:
```
 {
  "user": <logged-in user identifier>,
  "tasks": [
      {
        url:<setting url, MUST BE UNIQUE>,
        description:<setting description>,
      },
       .... 
   ]
 }
```

#### policy.js

The policy script takes as parameter the list of tasks from the user script output and saves it to the local storage. When the policy script is executed and local storage already contains the tasks list it takes one task and processes it.
To pass parameter to the policy script we should prepend the script with the definition of the params variable, which contains tasks, returned by user script: 

##### Policy script parameters:

```
const params = {
   "tasks": [ {
      "description": "Ads Preferences",
      "url": "https://x.com/settings/ads_preferences"
   } ]
}
;
<policy script content>
```

##### Policy script result:
The policy script returns the next object as result:

```
{
    "progress": "<percent of completion>",
    "applied_tasks": [{
      "description": "Ads Preferences",
      "url": "https://x.com/settings/ads_preferences",
      "error_description": "<optional error description>"
    }]
}
```
- `progress` - script calculates the percent of operation completion;
- `applied_tasks` - array of the completed tasks, each task is an object `url/description/error_description`


