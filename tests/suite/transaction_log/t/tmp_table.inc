# 
# Simple test of the serial event log for CREATE TEMPORARY calls 
# 
# Manipulates data between a temporary table and a non temporary table 
# 
# We then use the transaction_reader in drizzled/message/ to read the events.
#

--disable_warnings
DROP TABLE IF EXISTS t1,t2,t3;
--enable_warnings

CREATE TABLE t1 (
  id INT NOT NULL
, padding VARCHAR(200) NOT NULL
);

CREATE TEMPORARY TABLE t2 (
  id INT NOT NULL
, padding VARCHAR(200) NOT NULL
);

CREATE TEMPORARY TABLE t3 LIKE t1;

INSERT INTO t2 VALUES (1, "I love testing.");
INSERT INTO t2 VALUES (2, "I hate testing.");

INSERT INTO t1 SELECT * FROM t2;
