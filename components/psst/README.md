# Privacy Settings Selection for Sites Tool (PSST)

The feature helps tweak privacy settings on popular websites.

## Spec
https://docs.google.com/document/d/1ccnBWBV_KkknZpZYxcOXTwtfIiaSzeLS5dMd08N2pbs/edit?tab=t.0

# PSST Common Workflow

1. The user opens any website.
2. PSST feature detects that it is supported website (it means that browser loads PSST CRX component and find the appropriate user and policy scripts).
3. The browser injects the user script into the page. The primary goal of the user script is to ensure that the user is signed in to the current website and return a list of URLs where we should apply the privacy settings.
4. If the user is not signed in, we just break the flow.
5. If the user is signed in and we have a list of URLs, we will display a consent modal dialog. In this dialog, the user can choose their preferred privacy settings and track the progress of the operation.
6. When the user clicks the OK button, we inject the policy script and execute it.
7. The policy script saves the list of URLs (tasks) to local storage to be available after the next navigation, takes the first URL (URL_1), marks it as current, and navigates to it.
8. When navigation to URL_1 is complete, and it is a supported website, the user script is injected and executed. Then the same happens as in points #3,4.
9. Since the workflow is running and the consent dialog is visible, we inject the policy script.
10. Once injected, the policy script loads saved URLs (tasks) from local storage, takes the current' URL (see p. #7), and applies the privacy setting.
11. Once the privacy setting is applied, the policy script marks the current URL as applied, takes the next one from the available URL (task) list and marks it as current, saves all the info to local storage, and navigates to the new current one.
12. Then we enumerate each available URL in the list and do the same as in points #8-11.
13. When all tasks are completed, the user sees all status and progress information in the consent dialog.

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
    "status": "<STARTED|COMPLETED>",
    "progress": "<percent of completion>",
    "applied": [{
      "description": "Ads Preferences",
      "url": "https://x.com/settings/ads_preferences",
      "error_description": "<optional error description>"
    }]
}
```
- `status` - represents the flow's status, `started` or `completed`;
- `progress` - script calculates the percent of operation completion;
- `applied` - array of the completed tasks, each task is an object `url/description/error_description`


