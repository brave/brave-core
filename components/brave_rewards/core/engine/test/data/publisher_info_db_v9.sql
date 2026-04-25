BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "meta" (
	"key"	LONGVARCHAR NOT NULL UNIQUE,
	"value"	LONGVARCHAR,
	PRIMARY KEY("key")
);
CREATE TABLE IF NOT EXISTS "media_publisher_info" (
	"media_key"	TEXT NOT NULL UNIQUE,
	"publisher_id"	LONGVARCHAR NOT NULL,
	PRIMARY KEY("media_key"),
	CONSTRAINT "fk_media_publisher_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info_old"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "recurring_donation" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY("publisher_id"),
	CONSTRAINT "fk_recurring_donation_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info_old"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "activity_info" (
	"publisher_id"	LONGVARCHAR NOT NULL,
	"duration"	INTEGER NOT NULL DEFAULT 0,
	"visits"	INTEGER NOT NULL DEFAULT 0,
	"score"	DOUBLE NOT NULL DEFAULT 0,
	"percent"	INTEGER NOT NULL DEFAULT 0,
	"weight"	DOUBLE NOT NULL DEFAULT 0,
	"reconcile_stamp"	INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT "activity_unique" UNIQUE("publisher_id","reconcile_stamp"),
	CONSTRAINT "fk_activity_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info_old"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "server_publisher_info" (
	"publisher_key"	LONGVARCHAR NOT NULL UNIQUE,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"excluded"	INTEGER NOT NULL DEFAULT 0,
	"address"	TEXT NOT NULL,
	PRIMARY KEY("publisher_key")
);
CREATE TABLE IF NOT EXISTS "server_publisher_banner" (
	"publisher_key"	LONGVARCHAR NOT NULL UNIQUE,
	"title"	TEXT,
	"description"	TEXT,
	"background"	TEXT,
	"logo"	TEXT,
	PRIMARY KEY("publisher_key"),
	CONSTRAINT "fk_server_publisher_banner_publisher_key" FOREIGN KEY("publisher_key") REFERENCES "server_publisher_info"("publisher_key") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "server_publisher_links" (
	"publisher_key"	LONGVARCHAR NOT NULL,
	"provider"	TEXT,
	"link"	TEXT,
	CONSTRAINT "server_publisher_links_unique" UNIQUE("publisher_key","provider"),
	CONSTRAINT "fk_server_publisher_links_publisher_key" FOREIGN KEY("publisher_key") REFERENCES "server_publisher_info"("publisher_key") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "server_publisher_amounts" (
	"publisher_key"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	CONSTRAINT "server_publisher_amounts_unique" UNIQUE("publisher_key","amount"),
	CONSTRAINT "fk_server_publisher_amounts_publisher_key" FOREIGN KEY("publisher_key") REFERENCES "server_publisher_info"("publisher_key") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "publisher_info" (
	"publisher_id"	LONGVARCHAR NOT NULL UNIQUE,
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
	"type"	INTEGER NOT NULL,
	"month"	INTEGER NOT NULL,
	"year"	INTEGER NOT NULL,
	CONSTRAINT "fk_contribution_info_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "pending_contribution" (
	"publisher_id"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	"viewing_id"	LONGVARCHAR NOT NULL,
	"type"	INTEGER NOT NULL,
	CONSTRAINT "fk_pending_contribution_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "contribution_queue" (
	"contribution_queue_id"	INTEGER NOT NULL,
	"type"	INTEGER NOT NULL,
	"amount"	DOUBLE NOT NULL,
	"partial"	INTEGER NOT NULL DEFAULT 0,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("contribution_queue_id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "contribution_queue_publishers" (
	"contribution_queue_id"	INTEGER NOT NULL,
	"publisher_key"	TEXT NOT NULL,
	"amount_percent"	DOUBLE NOT NULL,
	CONSTRAINT "fk_contribution_queue_publishers_publisher_key" FOREIGN KEY("publisher_key") REFERENCES "publisher_info"("publisher_id"),
	CONSTRAINT "fk_contribution_queue_publishers_id" FOREIGN KEY("contribution_queue_id") REFERENCES "contribution_queue"("contribution_queue_id") ON DELETE CASCADE
);
INSERT INTO "meta" VALUES ('mmap_status','-1');
INSERT INTO "meta" VALUES ('last_compatible_version','1');
INSERT INTO "meta" VALUES ('version','9');
INSERT INTO "activity_info" VALUES ('basicattentiontoken.org',31,1,1.1358598545838,26,25.919327084376,1553423066);
INSERT INTO "activity_info" VALUES ('brave.com',20,2,1.07471534438942,25,24.5240629127033,1553423066);
INSERT INTO "activity_info" VALUES ('slo-tech.com',44,2,2.17171393564128,49,49.5566100029207,1553423066);
INSERT INTO "activity_info" VALUES ('3zsistemi.si',9,1,1.00662511542715,100,100.0,1573206313);
INSERT INTO "server_publisher_info" VALUES ('3zsistemi.si',2,0,'0fd15856-ada1-45a3-9ec7-74ac4db76eec');
INSERT INTO "server_publisher_info" VALUES ('laurenwags.github.io',2,0,'abf1ff79-a239-42af-abff-20eb121edd1c');
INSERT INTO "server_publisher_banner" VALUES ('laurenwags.github.io','Staging Banner Test','Lorem ipsum dolor sit amet, sale homero neglegentur ei vix, quo no tacimates vituperatoribus. Per elit luptatum temporibus ad, cibo minimum quaerendum no nec, atqui corpora complectitur te sed. Per ne vulputate neglegentur, id nec alia affert aperiri. Ea melius deserunt pro. Officiis sadipscing at nam, adhuc populo atomorum est.','chrome://rewards-image/https://rewards-stg.bravesoftware.com/xrEJASVGN9nQ5zJUnmoCxjEE','chrome://rewards-image/https://rewards-stg.bravesoftware.com/8eT9LXcpK3D795YHxvDdhrmg');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','twitch','https://www.twitch.tv/laurenwags');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','twitter','https://twitter.com/bravelaurenwags');
INSERT INTO "server_publisher_links" VALUES ('laurenwags.github.io','youtube','https://www.youtube.com/channel/UCCs7AQEDwrHEc86r0NNXE_A/videos');
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',5.0);
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',10.0);
INSERT INTO "server_publisher_amounts" VALUES ('laurenwags.github.io',20.0);
INSERT INTO "publisher_info" VALUES ('slo-tech.com',0,'slo-tech.com','','https://slo-tech.com/','');
INSERT INTO "publisher_info" VALUES ('brave.com',0,'brave.com','','https://brave.com/','');
INSERT INTO "publisher_info" VALUES ('basicattentiontoken.org',0,'basicattentiontoken.org','','https://basicattentiontoken.org/','');
INSERT INTO "publisher_info" VALUES ('reddit.com',0,'reddit.com','','https://www.reddit.com/','');
INSERT INTO "publisher_info" VALUES ('3zsistemi.si',0,'3zsistemi.si','','https://3zsistemi.si/','');
INSERT INTO "contribution_info" VALUES ('3zsistemi.si','1000000000000000000',1570614352,8,10,2019);
INSERT INTO "pending_contribution" VALUES ('reddit.com',1.0,1570614383,'',8);
CREATE INDEX IF NOT EXISTS "recurring_donation_publisher_id_index" ON "recurring_donation" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "activity_info_publisher_id_index" ON "activity_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "server_publisher_info_publisher_key_index" ON "server_publisher_info" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "server_publisher_banner_publisher_key_index" ON "server_publisher_banner" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "server_publisher_links_publisher_key_index" ON "server_publisher_links" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "server_publisher_amounts_publisher_key_index" ON "server_publisher_amounts" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "contribution_info_publisher_id_index" ON "contribution_info" (
	"publisher_id"
);
CREATE INDEX IF NOT EXISTS "pending_contribution_publisher_id_index" ON "pending_contribution" (
	"publisher_id"
);
COMMIT;
