# Privacy Settings Selection for Sites Tool (PSST)

The feature helps tweak privacy settings on popular websites.

## Spec
https://docs.google.com/document/d/1ccnBWBV_KkknZpZYxcOXTwtfIiaSzeLS5dMd08N2pbs/edit?tab=t.0


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
  "tasks": {
      [
        url:<setting url, MUST BE UNIQUE>,
        description:<setting description>,
      },
       .... 
   }
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

