/* Copyright (C) 2000-2005 MySQL AB && Innobase Oy

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA */

/*
  This file is based on ha_berkeley.h of MySQL distribution

  This file defines the Innodb handler: the interface between MySQL and
  Innodb
*/

#ifdef USE_PRAGMA_INTERFACE
#pragma interface			/* gcc class implementation */
#endif

typedef struct st_innobase_share {
  THR_LOCK lock;
  pthread_mutex_t mutex;
  char *table_name;
  uint32_t table_name_length,use_count;
} INNOBASE_SHARE;


struct dict_index_struct;
struct row_prebuilt_struct;

typedef struct dict_index_struct dict_index_t;
typedef struct row_prebuilt_struct row_prebuilt_t;

/* The class defining a handle to an Innodb table */
class ha_innobase: public handler
{
	row_prebuilt_t*	prebuilt;	/* prebuilt struct in InnoDB, used
					to save CPU time with prebuilt data
					structures*/
	THD*		user_thd;	/* the thread handle of the user
					currently using the handle; this is
					set in external_lock function */
	THR_LOCK_DATA	lock;
	INNOBASE_SHARE	*share;

	unsigned char*		upd_buff;	/* buffer used in updates */
	unsigned char*		key_val_buff;	/* buffer used in converting
					search key values from MySQL format
					to Innodb format */
	ulong		upd_and_key_val_buff_len;
					/* the length of each of the previous
					two buffers */
	Table_flags	int_table_flags;
	uint		primary_key;
	ulong		start_of_scan;	/* this is set to 1 when we are
					starting a table scan but have not
					yet fetched any row, else 0 */
	uint		last_match_mode;/* match mode of the latest search:
					ROW_SEL_EXACT, ROW_SEL_EXACT_PREFIX,
					or undefined */
	uint		num_write_row;	/* number of write_row() calls */

	uint32_t store_key_val_for_row(uint32_t keynr, char* buff, uint32_t buff_len,
                                   const unsigned char* record);
	int update_thd(THD* thd);
	int change_active_index(uint32_t keynr);
	int general_fetch(unsigned char* buf, uint32_t direction, uint32_t match_mode);
	int innobase_read_and_init_auto_inc(int64_t* ret);
	ulong innobase_autoinc_lock();
	ulong innobase_set_max_autoinc(uint64_t auto_inc);
	ulong innobase_reset_autoinc(uint64_t auto_inc);
	ulong innobase_get_auto_increment(uint64_t* value);
	dict_index_t* innobase_get_index(uint32_t keynr);

	/* Init values for the class: */
 public:
	ha_innobase(handlerton *hton, TABLE_SHARE *table_arg);
	~ha_innobase() {}
	/*
	  Get the row type from the storage engine.  If this method returns
	  ROW_TYPE_NOT_USED, the information in HA_CREATE_INFO should be used.
	*/
	enum row_type get_row_type() const;

	const char* table_type() const { return("InnoDB");}
        const char *index_type(uint32_t key_number __attribute__((unused)))
        { return "BTREE"; }
	const char** bas_ext() const;
	Table_flags table_flags() const;
	uint32_t index_flags(uint32_t idx __attribute__((unused)),
                             uint32_t part __attribute__((unused)),
                             bool all_parts __attribute__((unused))) const
	{
	  return (HA_READ_NEXT |
		  HA_READ_PREV |
		  HA_READ_ORDER |
		  HA_READ_RANGE |
		  HA_KEYREAD_ONLY | 
                  ((idx == primary_key)? 0 : HA_DO_INDEX_COND_PUSHDOWN));
	}
	uint32_t max_supported_keys()	   const { return MAX_KEY; }
				/* An InnoDB page must store >= 2 keys;
				a secondary key record must also contain the
				primary key value:
				max key length is therefore set to slightly
				less than 1 / 4 of page size which is 16 kB;
				but currently MySQL does not work with keys
				whose size is > MAX_KEY_LENGTH */
	uint32_t max_supported_key_length() const { return 3500; }
	uint32_t max_supported_key_part_length() const;
	const key_map *keys_to_use_for_scanning() { return &key_map_full; }

	int open(const char *name, int mode, uint32_t test_if_locked);
	int close(void);
	double scan_time();
	double read_time(uint32_t index, uint32_t ranges, ha_rows rows);

	int write_row(unsigned char * buf);
	int update_row(const unsigned char * old_data, unsigned char * new_data);
	int delete_row(const unsigned char * buf);
	bool was_semi_consistent_read();
	void try_semi_consistent_read(bool yes);
	void unlock_row();

	int index_init(uint32_t index, bool sorted);
	int index_end();
	int index_read(unsigned char * buf, const unsigned char * key,
		uint32_t key_len, enum ha_rkey_function find_flag);
	int index_read_idx(unsigned char * buf, uint32_t index, const unsigned char * key,
			   uint32_t key_len, enum ha_rkey_function find_flag);
	int index_read_last(unsigned char * buf, const unsigned char * key, uint32_t key_len);
	int index_next(unsigned char * buf);
	int index_next_same(unsigned char * buf, const unsigned char *key, uint32_t keylen);
	int index_prev(unsigned char * buf);
	int index_first(unsigned char * buf);
	int index_last(unsigned char * buf);

	int rnd_init(bool scan);
	int rnd_end();
	int rnd_next(unsigned char *buf);
	int rnd_pos(unsigned char * buf, unsigned char *pos);

	void position(const unsigned char *record);
	int info(uint);
	int analyze(THD* thd,HA_CHECK_OPT* check_opt);
	int optimize(THD* thd,HA_CHECK_OPT* check_opt);
	int discard_or_import_tablespace(bool discard);
	int extra(enum ha_extra_function operation);
        int reset();
        int lock_table(THD *thd, int lock_type, int lock_timeout)
        {
          /*
            Preliminarily call the pre-existing internal method for
            transactional locking and ignore non-transactional locks.
          */
          if (!lock_timeout)
          {
            /* Preliminarily show both possible errors for NOWAIT. */
            if (lock_type == F_WRLCK)
              return HA_ERR_UNSUPPORTED;
            else
              return HA_ERR_LOCK_WAIT_TIMEOUT;
          }
          return transactional_table_lock(thd, lock_type);
        }
	int external_lock(THD *thd, int lock_type);
	int transactional_table_lock(THD *thd, int lock_type);
	int start_stmt(THD *thd, thr_lock_type lock_type);
	void position(unsigned char *record);
	ha_rows records_in_range(uint32_t inx, key_range *min_key, key_range
								*max_key);
	ha_rows estimate_rows_upper_bound();

	void update_create_info(HA_CREATE_INFO* create_info);
	int create(const char *name, register Table *form,
					HA_CREATE_INFO *create_info);
	int delete_all_rows();
	int delete_table(const char *name);
	int rename_table(const char* from, const char* to);
	int check(THD* thd, HA_CHECK_OPT* check_opt);
	char* update_table_comment(const char* comment);
	char* get_foreign_key_create_info();
	int get_foreign_key_list(THD *thd, List<FOREIGN_KEY_INFO> *f_key_list);
	bool can_switch_engines();
	uint32_t referenced_by_foreign_key();
	void free_foreign_key_create_info(char* str);
	THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
					enum thr_lock_type lock_type);
	void init_table_handle_for_HANDLER();
        virtual void get_auto_increment(uint64_t offset, uint64_t increment,
                                        uint64_t nb_desired_values,
                                        uint64_t *first_value,
                                        uint64_t *nb_reserved_values);
	int reset_auto_increment(uint64_t value);

	virtual bool get_error_message(int error, String *buf);

	uint8_t table_cache_type() { return HA_CACHE_TBL_ASKTRANSACT; }
	/*
	  ask handler about permission to cache table during query registration
	*/
        bool register_query_cache_table(THD *thd, char *table_key,
                                        uint32_t key_length,
                                        qc_engine_callback *call_back,
                                        uint64_t *engine_data);
	static char *get_mysql_bin_log_name();
	static uint64_t get_mysql_bin_log_pos();
	bool primary_key_is_clustered() { return true; }
	int cmp_ref(const unsigned char *ref1, const unsigned char *ref2);
	bool check_if_incompatible_data(HA_CREATE_INFO *info,
					uint32_t table_changes);
public:
  /**
   * Multi Range Read interface
   */
  int multi_range_read_init(RANGE_SEQ_IF *seq, void *seq_init_param,
                            uint32_t n_ranges, uint32_t mode, HANDLER_BUFFER *buf);
  int multi_range_read_next(char **range_info);
  ha_rows multi_range_read_info_const(uint32_t keyno, RANGE_SEQ_IF *seq,
                                      void *seq_init_param, 
                                      uint32_t n_ranges, uint32_t *bufsz,
                                      uint32_t *flags, COST_VECT *cost);
  int multi_range_read_info(uint32_t keyno, uint32_t n_ranges, uint32_t keys,
                            uint32_t *bufsz, uint32_t *flags, COST_VECT *cost);
  DsMrr_impl ds_mrr;

  int read_range_first(const key_range *start_key, const key_range *end_key,
                       bool eq_range_arg, bool sorted);
  int read_range_next();
  Item *idx_cond_push(uint32_t keyno, Item* idx_cond);
};

/* Some accessor functions which the InnoDB plugin needs, but which
can not be added to mysql/plugin.h as part of the public interface;
the definitions are bracketed with #ifdef INNODB_COMPATIBILITY_HOOKS */

#ifndef INNODB_COMPATIBILITY_HOOKS
#error InnoDB needs MySQL to be built with #define INNODB_COMPATIBILITY_HOOKS
#endif

extern "C" {
struct charset_info_st *thd_charset(DRIZZLE_THD thd);
char **thd_query(DRIZZLE_THD thd);

/** Get the file name of the MySQL binlog.
 * @return the name of the binlog file
 */
const char* mysql_bin_log_file_name(void);

/** Get the current position of the MySQL binlog.
 * @return byte offset from the beginning of the binlog
 */
uint64_t mysql_bin_log_file_pos(void);

/**
  Check if a user thread is a replication slave thread
  @param thd  user thread
  @retval 0 the user thread is not a replication slave thread
  @retval 1 the user thread is a replication slave thread
*/
int thd_slave_thread(const DRIZZLE_THD thd);

/**
  Check if a user thread is running a non-transactional update
  @param thd  user thread
  @retval 0 the user thread is not running a non-transactional update
  @retval 1 the user thread is running a non-transactional update
*/
int thd_non_transactional_update(const DRIZZLE_THD thd);

/**
  Get the user thread's binary logging format
  @param thd  user thread
  @return Value to be used as index into the binlog_format_names array
*/
int thd_binlog_format(const DRIZZLE_THD thd);

/**
  Mark transaction to rollback and mark error as fatal to a sub-statement.
  @param  thd   Thread handle
  @param  all   TRUE <=> rollback main transaction.
*/
void thd_mark_transaction_to_rollback(DRIZZLE_THD thd, bool all);
}
