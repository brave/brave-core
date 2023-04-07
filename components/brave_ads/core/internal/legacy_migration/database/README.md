# Database Migration

Migrate legacy database state. You **MUST** increment `kVersion` and `kCompatibleVersion` in [database_constants.h](../../legacy_migration/database/database_constants.h) and add a corresponding `database_schema_{#}.sqlite` to `brave/components/brave_ads/core/test/data/`.

Please add to it!
