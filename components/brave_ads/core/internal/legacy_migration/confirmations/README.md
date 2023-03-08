# Confirmations Migration

Migrate legacy confirmation state. You **MUST** increment the version number for `kHasMigratedConfirmationState` in [pref_names.cc](../../../pref_names.cc) after making any changes to `confirmations.json` state.

**IMPORTANT:** Development and QA **MUST** re-test migration paths, and the `mutated` user data flag, see [README.md](../../account/user_data/README.md). Please test in Nightly before requesting uplifts to the beta or release channels.

Please add to it!
