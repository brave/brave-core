BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "meta" (
	"key"	LONGVARCHAR NOT NULL UNIQUE,
	"value"	LONGVARCHAR,
	PRIMARY KEY("key")
);
CREATE TABLE IF NOT EXISTS "activity_info" (
	"publisher_id"	LONGVARCHAR NOT NULL,
	"duration"	INTEGER NOT NULL DEFAULT 0,
	"score"	DOUBLE NOT NULL DEFAULT 0,
	"percent"	INTEGER NOT NULL DEFAULT 0,
	"weight"	DOUBLE NOT NULL DEFAULT 0,
	"category"	INTEGER NOT NULL,
	"month"	INTEGER NOT NULL,
	"year"	INTEGER NOT NULL,
	CONSTRAINT "fk_activity_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "media_publisher_info" (
	"media_key"	TEXT NOT NULL UNIQUE,
	"publisher_id"	LONGVARCHAR NOT NULL,
	PRIMARY KEY("media_key"),
	CONSTRAINT "fk_media_publisher_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "publisher_info" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
	"verified"	BOOLEAN NOT NULL DEFAULT 0,
	"excluded"	INTEGER NOT NULL DEFAULT 0,
	"name"	TEXT NOT NULL,
	"favIcon"	TEXT NOT NULL,
	"url"	TEXT NOT NULL,
	"provider"	TEXT NOT NULL,
	PRIMARY KEY("publisher_id")
);
INSERT INTO "meta" VALUES ('mmap_status','-1');
INSERT INTO "meta" VALUES ('version','1');
INSERT INTO "meta" VALUES ('last_compatible_version','1');
COMMIT;
