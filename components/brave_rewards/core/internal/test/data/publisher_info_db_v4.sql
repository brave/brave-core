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
	"visits"	INTEGER NOT NULL DEFAULT 0,
	"score"	DOUBLE NOT NULL DEFAULT 0,
	"percent"	INTEGER NOT NULL DEFAULT 0,
	"weight"	DOUBLE NOT NULL DEFAULT 0,
	"month"	INTEGER NOT NULL,
	"year"	INTEGER NOT NULL,
	"reconcile_stamp"	INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT "activity_unique" UNIQUE("publisher_id","month","year","reconcile_stamp"),
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
INSERT INTO "meta" VALUES ('mmap_status','-1');
INSERT INTO "meta" VALUES ('last_compatible_version','1');
INSERT INTO "meta" VALUES ('version','4');
INSERT INTO "publisher_info" VALUES ('brave.com',0,0,'brave.com','','https://brave.com/','');
INSERT INTO "publisher_info" VALUES ('slo-tech.com',0,0,'slo-tech.com','','https://slo-tech.com/','');
INSERT INTO "publisher_info" VALUES ('basicattentiontoken.org',0,0,'basicattentiontoken.org','','https://basicattentiontoken.org/','');
INSERT INTO "activity_info" VALUES ('brave.com',10,0,1.12593247244573,30,29.9456389581773,2,2019,1552214829);
INSERT INTO "activity_info" VALUES ('slo-tech.com',12,0,1.23982569647756,33,32.9747774279396,2,2019,1552214829);
INSERT INTO "activity_info" VALUES ('basicattentiontoken.org',15,3,1.39416318062023,37,37.0795836138831,2,2019,1552214829);
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
