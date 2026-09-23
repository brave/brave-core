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
	CONSTRAINT "fk_contribution_queue_publishers_id" FOREIGN KEY("contribution_queue_id") REFERENCES "contribution_queue"("contribution_queue_id") ON DELETE CASCADE,
	CONSTRAINT "fk_contribution_queue_publishers_publisher_key" FOREIGN KEY("publisher_key") REFERENCES "publisher_info"("publisher_id")
);
CREATE TABLE IF NOT EXISTS "promotion" (
	"promotion_id"	TEXT NOT NULL,
	"version"	INTEGER NOT NULL,
	"type"	INTEGER NOT NULL,
	"public_keys"	TEXT NOT NULL,
	"suggestions"	INTEGER NOT NULL DEFAULT 0,
	"approximate_value"	DOUBLE NOT NULL DEFAULT 0,
	"status"	INTEGER NOT NULL DEFAULT 0,
	"expires_at"	TIMESTAMP NOT NULL,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	"claimed_at"	TIMESTAMP,
	PRIMARY KEY("promotion_id")
);
CREATE TABLE IF NOT EXISTS "promotion_creds" (
	"promotion_id"	TEXT NOT NULL UNIQUE,
	"tokens"	TEXT NOT NULL,
	"blinded_creds"	TEXT NOT NULL,
	"signed_creds"	TEXT,
	"public_key"	TEXT,
	"batch_proof"	TEXT,
	"claim_id"	TEXT,
	CONSTRAINT "fk_promotion_creds_promotion_id" FOREIGN KEY("promotion_id") REFERENCES "promotion"("promotion_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "unblinded_tokens" (
	"token_id"	INTEGER NOT NULL,
	"token_value"	TEXT,
	"public_key"	TEXT,
	"value"	DOUBLE NOT NULL DEFAULT 0,
	"promotion_id"	TEXT,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("token_id" AUTOINCREMENT),
	CONSTRAINT "fk_unblinded_tokens_promotion_id" FOREIGN KEY("promotion_id") REFERENCES "promotion"("promotion_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "contribution_info_publishers" (
	"contribution_id"	TEXT NOT NULL,
	"publisher_key"	TEXT NOT NULL,
	"total_amount"	DOUBLE NOT NULL,
	"contributed_amount"	DOUBLE,
	CONSTRAINT "fk_contribution_info_publishers_publisher_id" FOREIGN KEY("publisher_key") REFERENCES "publisher_info"("publisher_id"),
	CONSTRAINT "fk_contribution_info_publishers_contribution_id" FOREIGN KEY("contribution_id") REFERENCES "contribution_info_temp"("contribution_id") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "contribution_info" (
	"contribution_id"	TEXT NOT NULL,
	"amount"	DOUBLE NOT NULL,
	"type"	INTEGER NOT NULL,
	"step"	INTEGER NOT NULL DEFAULT -1,
	"retry_count"	INTEGER NOT NULL DEFAULT -1,
	"created_at"	TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY("contribution_id")
);
CREATE TABLE IF NOT EXISTS "pending_contribution" (
	"pending_contribution_id"	INTEGER NOT NULL,
	"publisher_id"	LONGVARCHAR NOT NULL,
	"amount"	DOUBLE NOT NULL DEFAULT 0,
	"added_date"	INTEGER NOT NULL DEFAULT 0,
	"viewing_id"	LONGVARCHAR NOT NULL,
	"type"	INTEGER NOT NULL,
	PRIMARY KEY("pending_contribution_id" AUTOINCREMENT),
	CONSTRAINT "fk_pending_contribution_publisher_id" FOREIGN KEY("publisher_id") REFERENCES "publisher_info"("publisher_id") ON DELETE CASCADE
);
INSERT INTO "meta" VALUES ('mmap_status','-1');
INSERT INTO "meta" VALUES ('last_compatible_version','1');
INSERT INTO "meta" VALUES ('version','13');
INSERT INTO "activity_info" VALUES ('basicattentiontoken.org',31,1,1.1358598545838,26,25.919327084376,1553423066);
INSERT INTO "activity_info" VALUES ('brave.com',20,2,1.07471534438942,25,24.5240629127033,1553423066);
INSERT INTO "activity_info" VALUES ('slo-tech.com',44,2,2.17171393564128,49,49.5566100029207,1553423066);
INSERT INTO "activity_info" VALUES ('3zsistemi.si',9,1,1.00662511542715,100,100.0,1573206313);
INSERT INTO "activity_info" VALUES ('3zsistemi.si',15,1,1.04476927411666,100,100.0,1574671372);
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
INSERT INTO "publisher_info" VALUES ('bumpsmack.com',0,'bumpsmack.com','','https://bumpsmack.com/','');
INSERT INTO "publisher_info" VALUES ('jotunheimr.org',0,'jotunheimr.org','','http://jotunheimr.org/','');
INSERT INTO "publisher_info" VALUES ('3zsistemi.si',0,'3zsistemi.si','','https://3zsistemi.si/','');
INSERT INTO "promotion" VALUES ('36baa4c3-f92d-4121-b6d9-db44cb273a02',5,0,'["vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE="]',5,1.179110272,4,1583018235,'2020-01-08 15:42:27',0);
INSERT INTO "promotion_creds" VALUES ('36baa4c3-f92d-4121-b6d9-db44cb273a02','["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"]','["vrfDbBobhDwOcZ2/Nx2ztoGsx9ckVNiopL86R3BcSmc=","DjrkCHP+SUY0UNbIyGYPSgqliqpq4WxnCZ14R/ac13I=","Fv9omflWv3/L9oaL7QukGCwDxX0A/BTdgBaGnHGp7w0=","CgHvD0NBc08t9TnGRs45vJMxXH5GdkZknrs73etEbwc=","lux8l5DEll+kAOEF+HlwugDUm4nvn67+hXK4PCDnuhs=","bGZpbmisvqNdmJSpjgtopXmuTDPgTR9+JS8C/LLO6g8=","Wke8ZQM4+IP5YGf3qNZciwqt9xoTDjTIPcxrcNKwYn4=","/tiSGFiUCJA54Np1OUlxzK2Wcg4N1DRnYpYjGWxydkk=","qr06N/k1M70w85c+LOsZten+ejl2d4xnL6v7VxYzzFc=","5qegZeIPi23PpLHgzuXIR3Y60uMG9BJSIbbSC83sdXI=","kgHgPzuiHgSF1c8cSvAEQPuE8SBMAX9SdLR3CfW0SCI=","1IYdzLGgUlB3n2Ey4I1S0va9SK3j6P0kXRNZ62Tzl0A=","5ABHsBTb4qJkfqpxmlX52oXdpekigKy2L64SH/bmSmU=","bKgdDFqNNN2kBW0U45yqaFzZSWsxfpH97KNIK/Ip9m4=","4j4ZzgjYL0lnOrc/RdW9HS2dSj9FPf5uL+1gkbryLng=","KiqV5eIGClNTUbm0sqWEgbpKAa0S+e/+AdYg2YmoEVI=","NAOlqkhGEBnkQTt697PB4/geG+HUigb9DFNrFh+0JQ0=","sr453WWH2Og0SpYKkraMC+OIDT8fV+uFhuKWRpyC8w0=","PBnYdn0WgqaY4PNhlsNJzfbSrP2kYuvQLdaakKDeT0A=","VI/jnDqYMnebjTn0ACxYu9ImLpbFg725uIIF6bHMFm0=","6nTKfh1Fscumem0n+E12heNmDNEOxb3LHdm9T33arE4=","rLLelR1yW7sm21B4t4PQ+SCx/qg5T1xt5VJ308udol8=","IknFgUd+YFGaD7dJkdnIttS/KY8KUR8vKbXhmIdzD0o=","0iZTRcEqL7js+Ph1zrr7JrRA2DL0IpKGNDRFPdpakhs=","PPxvn8q8rVh0zgCwSdyCq1oINlunQWzwwA+WJhvKxig=","SuaNyI8grQXCW+TfgPgnXdx44K8SsTo3Br6z582OBW0=","oLDxobyhoonM9eZAvzB4hjyS63rgqssEZu0Z/vRiaSc=","kAXPSnDihSRnLbSA9dDBBDfBCo8AZOExAqjaXP8U8j0=","wm+SXRBo14oJmFJrXPy/0YMCI0wOYPyQTwZDWdbssiU=","Lqm4czy2VWs6MwkB6/G/0wOmKSiyjfshn8/C8gqaWnc=","0Gx/QpdZop1xLgKWMzMvR2+Pr+7zTMD5Pe6AGXotsUw=","ulcfEUVZa5Usx0sQqiAIEgDGnV4w3sK4ZS1Rw5IpukY=","Dk8NCtaKx0JbBgAuCWSeTg/SjzlIlK/Z9b2yoyvubDY=","Tm4qfoM3qvZYzRBY9Mnh9ROUEhP6AD1CkoubLD43Elk=","3g4+E4pj0nC9lKkgcWyPdAY+jLxedRf/e58uKzInRX0=","JvUPFNvTkVFcvgYHd20kFKudfBVS+jWvV8Du8/7CAGs=","hLhwvbIivqgtC5ySL/HojJ2se8IpjIiG/1V8IaerSFs=","9Ix9+dLKkQJjQeHmm71vfhGBXLBa2IGIRv5ydFveZVc=","nApXcgqbdn/BCbO5WbWVNeupBgVaXKBHhx5W+KF+9wM=","SiBMA5zawHt30EzJwYsw4bk8+ehbXddgbHIQcUPUM0Y=","tGB4YOgnVIjAlm2Bcz+AdbH/ARTdwScEMMLY6zAkyB8=","tmbfyV+krAkopoNQSzIesOVgLLJCkhkrMZD3WIRP3HA=","GKB4eW75T1KRfTIaO0unlRD3FKrSsCn8CT3lyhjM/jU=","UvO20+5KVr2ogiP/iVOvrjqsKP4ONEl5RWjATbzlPyQ=","xBVQvVJRnQC5TUT7D5bA1MJmCYYb78vQsf+DlMdBnWo=","LgygTb2Z/SwKqnPPeE/R9uGgOxtdczquEAnVsWw5liY=","1kAM7fLlUwIrMiWJDkxomvqW8CvfReyxU/AVs2Zfe38=","+JRtyEMe6BqRc8Ob33EMMN+EpBXPS/3C12wBm8QZ6W4=","OmdpqGjv/LdlASnobc6E8aTBDCvGtcrIAxCjRck1VBo=","Xl+gr9k56SY0oA+b6hcbchUunofkuWsEivc+9GBioVM=","opyzcAyI06U5E4dykvbPrTCcgVACGx7/z+ojAqZtemw=","0Gw8GTg4KlElWKKPWkgX3EDcOOLOnN/x7dM4g9x9QDc=","Zu0Y1hw+Tn194J6iTMaltcgzxXA2IAq0ZIkOqDo+izc=","dKiQd3YlEt6Bwp/inl8JfyIulagKhEGjUOumra9tl1U=","nEM6S30GASi/mc4VZA0wx9rO543QWGtwDGS17a4Lf24=","cKoG3A8m6QcT5x9NocyR57pA/OZVJVLSoi7qnKwUH0o=","DPiNgEh59JSQGqdiVAM4WA+YUr2OvoceEoXqXMitZgk=","zhc9Wx9NARSK0KQktAYPx9MdS2gTByh0TawRvC0ZnWM=","RrkpJMCBIdr8qLVs+R0/vBCv2P3wYUw1mcKrXpTOhiw=","FmbSQw0Nh57LNLFiftVPK8gUegLUOBbwIvNwLVW/R3A=","9EDCqyjUv9MN65iIRji8C3KOgNttqZFB4OnuHzzV5DA=","CPKmIaIpmSTxtAOm5XxmrC+pAWaDhBnW2EuzzLQv+m8=","fix6+LdF78YzxDWcQ1W6gz2h3VwhxfUx5cN8nuX8Uyk=","pOAw0z1F0z3u2qz4pz0cQbvriuTw3zTUHmW7IivdJC4=","rvFvp4zrjdi/zz4s3kCItCctBxmwja7eY/jQ3e9+t1o=","2JWoAGOJbsYLdQZF7ucz5wvwxAkSCi6a6YPHrya5NXE=","Bvhe8NpVSBMgt1yJNg2s1JqXmTdTmCRtF/VGBry/x0M=","MrwOGyjHkLTVWVTNhDmrtox08uP9tSfP/f86RFPIRSE=","+pG/rUjvzP0sMPQP8y7PFbU0ppfgOb5eddWzI0hs7Qw=","UE+IuosO3M+wY2r4GyxvXbxk5zWkLxe3G9MMU7q61zA="]','["MLAWD7pHCvqZ8xAhI8tJPTaF0bnQQZGQIpQqurfQxyw=","bGNqxekOve+/1NjeI0+JKWj9yyQbooRGMXNYgZwPVyA=","HhFjt2tRmAc6Nrz29wiM6ezCMqYpF7PEu0QX05zTu3c=","xgzxHA1MJHiamYcFym7LqBwJyBeB5/9DvsLnOdhXMB0=","huckW7ArzdO6ngdEgwVl6GOgIgdhn2ja8zefq1PGGgs=","RF4ld73JOjBoYkNa8EbNOHEsaKPycHts3Pne4JOb1BU=","DPAqmPk68YLBVFHvakZrDzttDzdRgWlxfFOw33m6WDk=","Zobrardi0Fbfqsb6T/px4zr33wDG/7jX2aRJWErT41c=","hstQg+9YxmVmKllZ+e9Wh2lf1SmhbfKD+PkCnnGgI1o=","lLGiZpikURlYvpK4P2GFErW51RJ9jN5q8BV3Vay5vVY=","3Anqu6ith0oNhB24rNwkNRTgBAsrQh4K3bxsefNOUjo=","3OOvvqX7TlB5663LCD3dbp01VVIKpc7fHea0vfZBmn4=","VDnKNKg+YGGMIr6RxBFs7S+1oIwirl1DUsktmFf2bmA=","SN5Q/6w66T5+8ghygHNi97uJluEc6WO3ZZKmz95DaG0=","OBv3pmjf8nR4xXsSv72JCOr0+EPqoI9SzPhIirAk0CA=","0g+6SNYI9/6GNfB6otm2YzXGTm/N7QnjSRKtRD8jhyc=","hNIiaWpclvCIH8FJtNQyejxfdiSW2w1JzAgoAFMgczk=","wPzLrCOCkEddfUSgBs31keZmvZkRkHER7CwFU4G53Ww=","Zmlfz5JBXCE7O4JNJtVnW4rEAxXOO9NwoePVru16ox0=","krkMqzuoYBe5LIYox6MxnKHD/yyLLxkUsJS4CbC6+Fs=","2EHaWCdKWzQblxERq9S7rEt8WJeaJuQkTFXx0oLakQg=","nlSH15EXjA/8YcNcG+bRhWSsZiRG/VWQBRJu2V1+YCY=","YmodnmSNMP8zco/SAnMkLW67Jid8Vhcn9bdyzFg55nY=","KD9EmkMuOL2thoZF/nX+J4QQCTcm2z6owAY6hjUwwiI=","2p96+kVyoFDlR9RMzAJUP5O9wxl+4yXJGXgwTRINnjM=","QK5kFJqfLS8a+qszqKDMIQO/Oad+Mqs9VEB2OgMj2Wk=","Tj3u8HcLD/oVdIJYHaw5AsO4zuYrAtCg24IkWYA3aHQ=","Zuev1fd7795gaTljw4kAjkfQhFTG/tSdsbLmeXvBih4=","uAFNHvS1cYQjq7dIh/6//uzqOSJ8I1qjwZN+WeqjUi0=","DoEkz11GxHSU5bVGct1xkAWyBSJrHfmYIFlK4cRk3wo=","5EbwREm41ENuu1wAULHHQwmKnCUUCxTLC3stsM5KbkU=","5iwHHPTZ6d1ol/6jd0b/4DqWKuFs/jd+jJUbslm0vDM=","Uu1lD0Wdmszb86JPLMSokRchg2JHUyfVT9aQGsDbdSI=","Ik1iQJhN4Cl1GwWOBeo9t0Uf9heskqkftodhY2+fZwU=","CA5eQ4zZGaY9qLyQu7vzHjJ0i+LY9xx4GzURBc7N2Qc=","UIwcxDn7Po4TBfwIx6sEDspyVSPVya7aoYEbrBX4F1s=","Xk25hpiBzV9FL6HBxTBupnAakTSR+2605OmV5MOMQTs=","zo8pjmv6PkA4/QmAuDLM2o45iGdsoINeLAPepMvX21A=","2vNlsWHD2AmdrXpSHg3uqOTKlzGY4gH/oxcO4Sg9FD4=","QocZDYOYQI9PuaCCMK2oF1tMzGtAB2pB9eoE4WTlNzg=","AIu+UDWJUsouqNOcNHlGea5wv/Zh3AiBfKT+bL8CICc=","1notbleNVYlcZl/NPTLYqgwbzqb7uR1yLsAij/mV8gA=","pO4IpFhinV/HELE6rZAyDQg9RUtgdG5CasFYO0753Uo=","eueyVEfXB6jHi5U2RH72Uqf7nPI7VgeoxL/N1he5Tk4=","KDcpGAPZb5CpWvpaTzRYLOVwqltI0JtulG3RF981rkk=","RpTbThDE7KRdiGEMH0chB98Xnh/U5eP9aUgHfXD5Hlg=","FP2sYPHGmgHd9HPAuPgw0z1Il5GhW7vRVzF5wHLLyRg=","EHeII/bPRI0xfQdKyTNXEXpKytcCpumyucpGEuuXcnM=","ci026FdZaEqKyjIW8rCD/URSVph00o6iDIOKGgCfVDw=","jpiz9qPrWZDfe/3sD3NAG32fhT/AOOOmFpxksBMHqQc=","KrLPDxgdW0qHDJgRUiyb/qP38PPP1Gsh/MKX1P9jpSM=","hHhGBecXeWkafuXJAij08MtrNviOgMf+s++h+73Eyws=","pBU9B9yDLO8nzj6Z1M9T0Csz6ntqqQ9rcyRQdJWosFE=","Khoj1Ula8Sq/f7OzQUZyXBOfNaGnaODbOamL/HlNSEE=","DofbZQBZcCJEI+nLmDg4nIjZdLZs5GuRLSQAsV3Q9Bs=","Xo5AJ20Q0SMyF+hJbIc//ui/XVXcgCzH9NDRAg7jtVg=","Tjx5y0RpbOLNrEOonTEahjJKbQzJHyFS12t6xDUSvA4=","lKJSXjcNb0TZmjt40YhFG6VIisjiebjt7LniTIKaE08=","SPu3bK4/HlvtIWuqvaDPSNBSd+tWnDOicmQB/F+U9mQ=","+B/qshgV/5tblPIpGVLG/trPF/0jYNmEkreUQvPcaT8=","qAJXyg5FZvZ3pDFa4ugDSYOnqgSs6H04ctigmvLt7A0=","KL7on7rJIu0zetbSqf8J1qxFLo+sqoIZFPNhyfYtAFg=","nO7MKDXmlU6cKQFp0OqAudxeVIHbf01JwAX1sSTsYGY=","aF8J/ttmIfLKb7htl+9SgACtDvmDCbnwXQJXzqsaBxs=","HCQp07l5y/fHZ084hI7EjJDW4dVkTpzwxarCPkfQzGY=","SCJj2lkKG2KdX1thO86Fx64X4hGv9jn+aGbcH/Z0jB8=","zug4T7e8FqYJ3oGRkorMe0moV40WxOU/dtZ52qqPXk8=","UHO4NmSJBVPEjTzGR+nmt9JBLr6TbN20latCxipjuDU=","2MiQn7Mx45RKFUQnsxiOPsrlREMioGTSlBwI6s5rfis=","/AG8yD40x3H1T4nPUAsu/UbmEJrXg+LBdnIrmyq903Y="]','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=','9ueOTju3OLogE0EBhiVzVlwrySNj3bQGXOba8LLfTwCufZHFfaduvkQ1Ng4StLt2NK1RXY36wfC2OIGtRU6UDQ==','402afe8d-a643-4b8c-aa1e-596e3bcc6c8a');
INSERT INTO "unblinded_tokens" VALUES (69,'sPYS559Ny6r0jLaT/IMtL+791rULAeSoKQ53lT5rncZmEFifrvsRb3Xv+35e8+4lNcQ1QR/t6NfwsuhVIe/SjmAiM6V7bAHgG11ZPzsmZkA0Qfg91AOjvqpfzKDKJwRW','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=',0.25,'36baa4c3-f92d-4121-b6d9-db44cb273a02','2019-11-25 08:40:52');
INSERT INTO "unblinded_tokens" VALUES (70,'1zqdiaBG2AEHPkPvZsW9VyGfFoElG6pGqM2E4UknCT6cyvv4yhbOYxzUIosVVH0H/A75LRXE9Kmo/7TUQhVokk7qk8hEmv7XHhfUjHxO1mKcsaTSQr2mku/YFlsCjID4l','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=',0.25,'36baa4c3-f92d-4121-b6d9-db44cb273a02','2019-11-25 08:40:52');
INSERT INTO "unblinded_tokens" VALUES (71,'2zqdiaBG2AEHPkPvZsW9VyGfFoElG6pGqM2E4UknCT6cyvv4yhbOYxzUIosVVH0H/A75LRXE9Kmo/7TUQhVokk7qk8hEmv7XHhfUjHxO1mKcsaTSQr2mku/YFlsCjID4l','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=',0.2346456456,'36baa4c3-f92d-4121-b6d9-db44cb273a02','2019-11-25 08:40:52');
INSERT INTO "unblinded_tokens" VALUES (72,'3zqdiaBG2AEHPkPvZsW9VyGfFoElG6pGqM2E4UknCT6cyvv4yhbOYxzUIosVVH0H/A75LRXE9Kmo/7TUQhVokk7qk8hEmv7XHhfUjHxO1mKcsaTSQr2mku/YFlsCjID4l','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=',0.2222323132,'36baa4c3-f92d-4121-b6d9-db44cb273a02','2019-11-25 08:40:52');
INSERT INTO "unblinded_tokens" VALUES (73,'4zqdiaBG2AEHPkPvZsW9VyGfFoElG6pGqM2E4UknCT6cyvv4yhbOYxzUIosVVH0H/A75LRXE9Kmo/7TUQhVokk7qk8hEmv7XHhfUjHxO1mKcsaTSQr2mku/YFlsCjID4l','vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=',0.2222323132,'36baa4c3-f92d-4121-b6d9-db44cb273a02','2019-11-25 08:40:52');
INSERT INTO "contribution_info_publishers" VALUES ('id_1570614352_0','3zsistemi.si',1.0,1.0);
INSERT INTO "contribution_info_publishers" VALUES ('id_1574671265_1','3zsistemi.si',1.0,1.0);
INSERT INTO "contribution_info_publishers" VALUES ('id_1574671276_2','3zsistemi.si',5.0,5.0);
INSERT INTO "contribution_info_publishers" VALUES ('id_1574671293_3','bumpsmack.com',1.0,1.0);
INSERT INTO "contribution_info" VALUES ('id_1570614352_0',1.0,8,-1,-1,1570614352);
INSERT INTO "contribution_info" VALUES ('id_1574671265_1',1.0,8,-1,-1,1574671265);
INSERT INTO "contribution_info" VALUES ('id_1574671276_2',5.0,8,-1,-1,1574671276);
INSERT INTO "contribution_info" VALUES ('id_1574671293_3',1.0,8,-1,-1,1574671293);
INSERT INTO "contribution_info" VALUES ('id_1574671381_4',10.0,2,-1,-1,1574671381);
INSERT INTO "pending_contribution" VALUES (1,'reddit.com',1.0,1570614383,'',8);
INSERT INTO "pending_contribution" VALUES (2,'slo-tech.com',5.0,1570614383,'',8);
INSERT INTO "pending_contribution" VALUES (3,'slo-tech.com',10.0,1570714383,'',8);
INSERT INTO "pending_contribution" VALUES (4,'reddit.com',1.0,1570614383,'',8);
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
CREATE INDEX IF NOT EXISTS "promotion_promotion_id_index" ON "promotion" (
	"promotion_id"
);
CREATE INDEX IF NOT EXISTS "promotion_creds_promotion_id_index" ON "promotion_creds" (
	"promotion_id"
);
CREATE INDEX IF NOT EXISTS "unblinded_tokens_token_id_index" ON "unblinded_tokens" (
	"token_id"
);
CREATE INDEX IF NOT EXISTS "contribution_info_publishers_contribution_id_index" ON "contribution_info_publishers" (
	"contribution_id"
);
CREATE INDEX IF NOT EXISTS "contribution_info_publishers_publisher_key_index" ON "contribution_info_publishers" (
	"publisher_key"
);
CREATE INDEX IF NOT EXISTS "pending_contribution_publisher_id_index" ON "pending_contribution" (
	"publisher_id"
);
COMMIT;
