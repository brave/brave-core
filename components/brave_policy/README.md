There are 2 types of policy providers.

- BraveBrowserPolicyProvider: There is only one of these providers and it manages policies that are stored globally in local state.
- BraveProfilePolicyProvider: There is one of these providers per profile and it manages policies that are stored in profile prefs.

Policies are defined in `components/policy/resources/templates/policy_definitions/BraveSoftware/` and the YAML file has a `per_profile` attribute which associates it accordingly to the browser or profile provider.
Both of these providers will be used for Brave Origin, but also for other work within Brave.
