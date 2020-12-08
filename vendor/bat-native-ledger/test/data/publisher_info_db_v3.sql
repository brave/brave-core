BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "meta" (
	"key"	LONGVARCHAR NOT NULL UNIQUE,
	"value"	LONGVARCHAR,
	PRIMARY KEY("key")
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
CREATE TABLE IF NOT EXISTS "contribution_info" (
	"publisher_id"	LONGVARCHAR,
	"probi"	TEXT NOT NULL,
	"date"	INTEGER NOT NULL,
	"category"	INTEGER NOT NULL,
	"month"	INTEGER NOT NULL,
	"year"	INTEGER NOT NULL,
	CONSTRAINT "fk_contribution_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
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
	"reconcile_stamp"	INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT "fk_activity_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "media_publisher_info" (
	"media_key"	TEXT NOT NULL UNIQUE,
	"publisher_id"	LONGVARCHAR NOT NULL,
	PRIMARY KEY("media_key"),
	CONSTRAINT "fk_media_publisher_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "recurring_donation" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("publisher_id"),
	CONSTRAINT "fk_recurring_donation_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "pending_contribution" (
	"publisher_id"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	"viewing_id"	LONGVARCHAR NOT NULL,
	"category"	INTEGER NOT NULL,
	CONSTRAINT "fk_pending_contribution_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
INSERT INTO "meta" VALUES ('mmap_status','-1'),
 ('last_compatible_version','1'),
 ('version','3');
INSERT INTO "publisher_info" VALUES ('slo-tech.com',0,0,'slo-tech.com','','https://slo-tech.com/',''),
 ('brave.com',1,0,'brave.com','','https://brave.com/','');
INSERT INTO "activity_info" VALUES ('slo-tech.com',16,0.014523479751379,53,0.726173987568949,2,4,2019,1557639510),
 ('brave.com',14,0.012845035450015,47,0.64225177250075,2,4,2019,1557639510);
CREATE INDEX IF NOT EXISTS "contribution_info_publisher_id_index" ON "contribution_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "activity_info_publisher_id_index" ON "activity_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "recurring_donation_publisher_id_index" ON "recurring_donation" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "pending_contribution_publisher_id_index" ON "pending_contribution" (
	"publisher_id"
);
COMMIT;
