# Policy Best Practices

## Adding a new policy

Add an entry to `kBraveSimplePolicyMap` in
`browser/policy/brave_simple_policy_map.h`, mapping the policy key to its
corresponding pref name and value type.

## Policies that don't support dynamic refresh

If a policy cannot apply changes at runtime without a restart (i.e. the pref
value must be preserved across dynamic policy updates), also add the pref name
to `kNonDynamicPrefs` in the same file.

Prefer supporting dynamic refresh where possible.
