# Database Migration

Migrate legacy database state. You **MUST** increment `kVersionNumber` and `kCompatibleVersionNumber` in [database_constants.h](../../legacy_migration/database/database_constants.h) and add a corresponding `database_schema_{#}.sqlite` file to `brave/components/brave_ads/test/data/database/migration`.

See [database migration test data](../../../test/data/database/migration/README.md) on how to mock test data.

Please add to it!
