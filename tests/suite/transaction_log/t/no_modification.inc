# 
# Simple test of the transaction log for when a START TRANSACTION ... COMMIT
# is issued, but no data modifications actually occurred.
# 
# This situation occurs in, for instance, sysbench, which wraps even SELECTs
# in a transaction.
#

--disable_warnings
DROP TABLE IF EXISTS t1;
--enable_warnings

CREATE TABLE t1 (
  id INT NOT NULL
, padding VARCHAR(200) NOT NULL
);

INSERT INTO t1 VALUES (1, "I love testing.");
INSERT INTO t1 VALUES (2, "I hate testing.");

START TRANSACTION;

SELECT * FROM t1 WHERE id = 1;
SELECT * FROM t1 WHERE id = 2;

COMMIT;