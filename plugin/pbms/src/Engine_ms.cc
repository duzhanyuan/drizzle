/* Copyright (c) 2008 PrimeBase Technologies GmbH, Germany
 *
 * PrimeBase Media Stream for MySQL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Original author: Paul McCullagh
 * Continued development: Barry Leslie
 *
 * 2007-07-20
 *
 * H&G2JCtL
 *
 * Engine interface.
 *
 */

#include "cslib/CSConfig.h"
#include "cslib/CSGlobal.h"
#include "cslib/CSStrUtil.h"
#include "cslib/CSThread.h"

#define PBMS_API	pbms_api_PBMS

#include "Engine_ms.h"
#include "ConnectionHandler_ms.h"
#include "OpenTable_ms.h"
#include "Util_ms.h"
#include "Network_ms.h"
#include "Transaction_ms.h"
#include "ms_mysql.h"

// If PBMS support is built directly into the mysql/drizzle handler code 
// then calls from all other handlers are ignored.
static bool have_handler_support = false; 

/*
 * ---------------------------------------------------------------
 * ENGINE CALL-IN INTERFACE
 */

static PBMS_API *StreamingEngines;

#ifdef new
#undef new
#endif


static MSOpenTable *local_open_table(const char *db_name, const char *tab_name, bool create)
{
	MSOpenTable		*otab = NULL;
	uint32_t db_id, tab_id;
	enter_();
	
	if ( MSDatabase::convertTableAndDatabaseToIDs(db_name, tab_name, &db_id, &tab_id, create))  
		otab = MSTableList::getOpenTableByID(db_id, tab_id);
		
	return_(otab);
}


/*
 * ---------------------------------------------------------------
 * ENGINE CALLBACK INTERFACE
 */

static void ms_register_engine(PBMSEnginePtr engine)
{
	if (engine->ms_internal)
		have_handler_support = true;
}

static void ms_deregister_engine(PBMSEnginePtr engine __attribute__((unused)))
{
}

static int ms_create_blob(bool internal, const char *db_name, const char *tab_name, char *blob, size_t blob_len, char *blob_url, PBMSResultPtr result)
{
	CSThread		*self;
	int				err = MS_OK;
	MSOpenTable		*otab;
	CSInputStream	*i_stream = NULL;
	
	if (have_handler_support && !internal) {
		pbms_error_result(CS_CONTEXT, MS_ERR_INVALID_OPERATION, "Invalid ms_create_blob() call", result);
		return MS_ERR_ENGINE;
	}

	if ((err = pbms_enter_conn_no_thd(&self, result)))
		return err;

	inner_();
	try_(a) {
		otab = local_open_table(db_name, tab_name, true);
		frompool_(otab);
		
		if (!otab->getDB()->isRecovering()) {
			i_stream = CSMemoryInputStream::newStream((unsigned char *)blob, blob_len);
			otab->createBlob((PBMSBlobURLPtr)blob_url, blob_len, NULL, 0, i_stream);
		} else
			CSException::throwException(CS_CONTEXT, MS_ERR_RECOVERY_IN_PROGRESS, "Cannot create BLOBs during repository recovery.");

		backtopool_(otab);
	}
	catch_(a) {
		err = pbms_exception_to_result(&self->myException, result);
	}
	cont_(a);
	return_(err);
}

/*
 * ms_use_blob() may or may not alter the blob url depending on the type of URL and if the BLOB is in a
 * different database or not. It may also add a BLOB reference to the BLOB table log if the BLOB was from
 * a different table or no table was specified when the BLOB was uploaded.
 *
 * There is no need to undo this function because it will be undone automaticly if the BLOB is not retained.
 */
static int ms_retain_blob(bool internal, const char *db_name, const char *tab_name, char *ret_blob_url, char *blob_url, unsigned short col_index, PBMSResultPtr result)
{
	CSThread		*self;
	int				err = MS_OK;
	MSBlobURLRec	blob;
	MSOpenTable		*otab;
	
	if (have_handler_support && !internal) {
		cs_strcpy(PBMS_BLOB_URL_SIZE, ret_blob_url, blob_url); // This should have already been converted.
		return MS_OK;
	}
	
	if ((err = pbms_enter_conn_no_thd(&self, result)))
		return err;

	inner_();
	try_(a) {
		
		if (! ms_parse_blob_url(&blob, blob_url)){
			char buffer[CS_EXC_MESSAGE_SIZE];
			
			cs_strcpy(CS_EXC_MESSAGE_SIZE, buffer, "Incorrect URL: ");
			cs_strcat(CS_EXC_MESSAGE_SIZE, buffer, blob_url);
			CSException::throwException(CS_CONTEXT, MS_ERR_INCORRECT_URL, buffer);
		}
		
		otab = local_open_table(db_name, tab_name, true);
		frompool_(otab);

		otab->useBlob(blob.bu_type, blob.bu_db_id, blob.bu_tab_id, blob.bu_blob_id, blob.bu_auth_code, col_index, blob.bu_blob_size, blob.bu_blob_ref_id, ret_blob_url);

		backtopool_(otab);
	}
	catch_(a) {
		err = pbms_exception_to_result(&self->myException, result);
	}
	cont_(a);
	return_(err);
}

static int ms_release_blob(bool internal, const char *db_name, const char *tab_name, char *blob_url, PBMSResultPtr result)
{
	CSThread		*self;
	int				err = MS_OK;
	MSBlobURLRec	blob;
	MSOpenTable		*otab;

	if (have_handler_support && !internal) 
		return MS_OK;
	
	if ((err = pbms_enter_conn_no_thd(&self, result)))
		return err;

	inner_();
	try_(a) {
		if (! ms_parse_blob_url(&blob, blob_url)){
			char buffer[CS_EXC_MESSAGE_SIZE];

			cs_strcpy(CS_EXC_MESSAGE_SIZE, buffer, "Incorrect URL: ");
			cs_strcat(CS_EXC_MESSAGE_SIZE, buffer, blob_url);
			CSException::throwException(CS_CONTEXT, MS_ERR_INCORRECT_URL, buffer);
		}
		
		otab = local_open_table(db_name, tab_name, true);
		frompool_(otab);
		if (!otab->getDB()->isRecovering()) {
			if (otab->getTableID() == blob.bu_tab_id)
				otab->releaseReference(blob.bu_blob_id, blob.bu_blob_ref_id);
			else {
				char buffer[CS_EXC_MESSAGE_SIZE];

				cs_strcpy(CS_EXC_MESSAGE_SIZE, buffer, "Incorrect table ID: ");
				cs_strcat(CS_EXC_MESSAGE_SIZE, buffer, blob_url);
				CSException::throwException(CS_CONTEXT, MS_ERR_INCORRECT_URL, buffer);
			}
		}
		else {
			char buffer[CS_EXC_MESSAGE_SIZE];

			cs_strcpy(CS_EXC_MESSAGE_SIZE, buffer, "Incorrect URL: ");
			cs_strcat(CS_EXC_MESSAGE_SIZE, buffer, blob_url);
			CSException::throwException(CS_CONTEXT, MS_ERR_INCORRECT_URL, buffer);
		}
		
		backtopool_(otab);
		
		
	}
	catch_(a) {
		err = pbms_exception_to_result(&self->myException, result);
	}
	cont_(a);
	return_(err);
}

typedef struct {
	bool udo_WasRename;
	CSString *udo_DatabaseName;
	CSString *udo_OldName;
	CSString *udo_NewName;
} UnDoInfoRec, *UnDoInfoPtr;

static int ms_drop_table(bool internal, const char *db_name, const char *tab_name, PBMSResultPtr result)
{
	CSThread	*self;
	int			err;

	if (have_handler_support && !internal) 
		return MS_OK;

	if ((err = pbms_enter_conn_no_thd(&self, result)))
		return err;

	inner_();
	try_(a) {

		CSPath			*new_path;
		CSPath			*old_path;
		MSOpenTable		*otab;
		MSOpenTablePool	*tab_pool;
		MSTable			*tab;
		UnDoInfoPtr		undo_info = NULL;

		otab = local_open_table(db_name, tab_name, false);
		if (!otab)
			goto exit;
		
		// If we are recovering do not delete the table.
		// It is normal for MySQL recovery scripts to delete any table they aare about to
		// recover and then recreate it. If this is done after the repository has been recovered
		// then this would delete all the recovered BLOBs in the table.
		if (otab->getDB()->isRecovering()) {
			otab->returnToPool();
			goto exit;
		}

		frompool_(otab);

		// Before dropping the table the table ref file is renamed so that
		// it is out of the way incase a new table is created before the
		// old one is cleaned up.
		
		old_path = otab->getDBTable()->getTableFile();
		push_(old_path);

		new_path = otab->getDBTable()->getTableFile(tab_name, true);

		// Rearrage the object stack to pop the otab object
		pop_(old_path);
		pop_(otab);

		push_(new_path);
		push_(old_path);
		frompool_(otab);
		
		tab = otab->getDBTable();
		pop_(otab);
		push_(tab);

		tab_pool = MSTableList::lockTablePoolForDeletion(otab);
		frompool_(tab_pool);

		if (old_path->exists())
			old_path->move(new_path);
		tab->myDatabase->dropTable(RETAIN(tab));
		
		/* Add the table to the temp delete list if we are not recovering... */
		tab->prepareToDelete();

		backtopool_(tab_pool);	// The will unlock and close the table pool freeing all tables in it.
		pop_(tab);				// Returning the pool will have released this. (YUK!)
		release_(old_path);
		release_(new_path);


		undo_info = (UnDoInfoPtr) cs_malloc(sizeof(UnDoInfoRec));
		
		undo_info->udo_WasRename = false;
		self->myInfo = undo_info;
		
		
exit: ;
	}
	
	catch_(a) {
		err = pbms_exception_to_result(&self->myException, result);
	}
	cont_(a);
	outer_();
	pbms_exit_conn();
	return err;
}

static void complete_delete(UnDoInfoPtr info, bool ok)
{
	// TO DO: figure out a way to undo the delete.
	cs_free(info);
	if (!ok) 
		CSException::throwException(CS_CONTEXT, MS_ERR_NOT_IMPLEMENTED, "Cannot undo delete table.");
}

static bool my_rename_table(const char * db_name, const char *from_table, const char *to_table)
{
	MSOpenTable		*otab;
	CSPath			*from_path;
	CSPath			*to_path;
	MSOpenTablePool	*tab_pool;
	MSTable			*tab;

	enter_();
	
	otab = local_open_table(db_name, from_table, false);
	if (!otab)
		return_(false);
		
	frompool_(otab);

	if (otab->getDB()->isRecovering()) 
		CSException::throwException(CS_CONTEXT, MS_ERR_RECOVERY_IN_PROGRESS, "Cannot rename tables during repository recovery.");

	from_path = otab->getDBTable()->getTableFile();
	push_(from_path);

	to_path = otab->getDBTable()->getTableFile(to_table, false);

	// Rearrage the object stack to pop the otab object
	pop_(from_path);
	pop_(otab);

	push_(to_path);
	push_(from_path);
	frompool_(otab);

	otab->openForReading();
	tab = otab->getDBTable();
	tab->retain();
	pop_(otab);
	push_(tab);
	
	tab_pool = MSTableList::lockTablePoolForDeletion(otab);
	frompool_(tab_pool);

	from_path->move(to_path);
	tab->myDatabase->renameTable(tab, to_table);

	backtopool_(tab_pool);	// The will unlock and close the table pool freeing all tables in it.
	pop_(tab);				// Returning the pool will have released this. (YUK!)
	release_(from_path);
	release_(to_path);
	
	return_(true);
}

static int ms_rename_table(bool internal, const char * db_name, const char *from_table, const char *to_table, PBMSResultPtr result)
{
	CSThread	*self;
	int			err;
	UnDoInfoPtr undo_info = NULL;

	if (have_handler_support && !internal) 
		return MS_OK;

	if ((err = pbms_enter_conn_no_thd(&self, result)))
		return err;

	inner_();
	try_(a) {
		undo_info = (UnDoInfoPtr) cs_malloc(sizeof(UnDoInfoRec));

		undo_info->udo_WasRename = true;
		if (my_rename_table(db_name, from_table, to_table)) {		
			undo_info->udo_DatabaseName = CSString::newString(db_name);
			undo_info->udo_OldName = CSString::newString(from_table);
			undo_info->udo_NewName = CSString::newString(to_table);
		} else {
			undo_info->udo_DatabaseName = undo_info->udo_OldName = undo_info->udo_NewName = NULL;
		}
		self->myInfo = undo_info;
	}
	catch_(a) {
		err = pbms_exception_to_result(&self->myException, result);
		if (undo_info)
			cs_free(undo_info);
	}
	cont_(a);
	outer_();
	pbms_exit_conn();
	return err;
}

static void complete_rename(UnDoInfoPtr info, bool ok)
{
	// Swap the paths around here to revers the rename.
	CSString		*db_name= info->udo_DatabaseName;
	CSString		*from_table= info->udo_NewName;
	CSString		*to_table= info->udo_OldName;
	
	enter_();
	
	cs_free(info);
	if (db_name) {
		push_(db_name);
		push_(from_table);
		push_(to_table);
		if (!ok) 
			my_rename_table(db_name->getCString(), from_table->getCString(), to_table->getCString());
			
		release_(to_table);
		release_(from_table);
		release_(db_name);
	}
	exit_();
}

static void ms_completed(bool internal, bool ok)
{
	CSThread	*self;
	PBMSResultRec	result;
	
	if (have_handler_support && !internal) 
		return;

	if (pbms_enter_conn_no_thd(&self, &result))
		return ;

	if (self->myInfo) {
		UnDoInfoPtr info = (UnDoInfoPtr) self->myInfo;
			if (info->udo_WasRename) 
			complete_rename(info, ok);
		else
			complete_delete(info, ok);
		
		self->myInfo = NULL;
	} else if (self->myTID && (self->myIsAutoCommit || !ok)) {
		inner_();
		try_(a) {
			if (ok)
				MSTransactionManager::commit();
			else if (self->myIsAutoCommit)
				MSTransactionManager::rollback();
			else
				MSTransactionManager::rollbackToPosition(self->myStartStmt); // Rollback the last logical statement.
		}
		catch_(a) {
			self->logException();
		}
		cont_(a);
		outer_();
	}
	
	self->myStartStmt = self->myStmtCount;
}

PBMSCallbacksRec engine_callbacks = {
	MS_CALLBACK_VERSION,
	ms_register_engine,
	ms_deregister_engine,
	ms_create_blob,
	ms_retain_blob,
	ms_release_blob,
	ms_drop_table,
	ms_rename_table,
	ms_completed
};

// =============================
int MSEngine::startUp(PBMSResultPtr result)
{
	int err;
	
	StreamingEngines = new PBMS_API();
	err = StreamingEngines->PBMSStartup(&engine_callbacks, result);
	if (err)
		delete StreamingEngines;
	else { // Register the PBMS enabled engines the startup before PBMS
		PBMSSharedMemoryPtr		sh_mem = StreamingEngines->sharedMemory;
		PBMSEnginePtr			engine;
		
		for (int i=0; i<sh_mem->sm_list_len; i++) {
			if ((engine = sh_mem->sm_engine_list[i])) 
				ms_register_engine(engine);
		}
	}
	
	return err;
}

void MSEngine::shutDown()
{
	StreamingEngines->PBMSShutdown();

	delete StreamingEngines;
}

const PBMSEnginePtr MSEngine::getEngineInfoAt(int indx)
{
	PBMSSharedMemoryPtr		sh_mem = StreamingEngines->sharedMemory;
	PBMSEnginePtr			engine = NULL;
	
	if (sh_mem) {
		for (int i=0; i<sh_mem->sm_list_len; i++) {
			if ((engine = sh_mem->sm_engine_list[i])) {
				if (!indx)
					return engine;
				indx--;
			}
		}
	}
	
	return NULL;
}


