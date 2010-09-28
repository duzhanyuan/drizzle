SELECT server_id, transaction_id, num_statements FROM DATA_DICTIONARY.TRANSACTION_LOG_TRANSACTIONS;

--echo
--replace_regex /start_timestamp: [0-9]+/START_TIMESTAMP/g /end_timestamp: [0-9]+/END_TIMESTAMP/g /creation_timestamp: [0-9]+/CREATE_TIMESTAMP/ /update_timestamp: [0-9]+/UPDATE_TIMESTAMP/

SELECT PRINT_TRANSACTION_MESSAGE('transaction.log', ENTRY_OFFSET) FROM DATA_DICTIONARY.TRANSACTION_LOG_ENTRIES;