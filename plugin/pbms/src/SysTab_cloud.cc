/* Copyright (c) 2009 PrimeBase Technologies GmbH, Germany
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
 * Barry Leslie
 *
 * 2009-10-21
 *
 * System cloud starage info table.
 *
 */
#ifdef DRIZZLED
#include "config.h"
#include <drizzled/common.h>
#include <drizzled/session.h>
#endif

#include "cslib/CSConfig.h"
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

//#include "mysql_priv.h"
#include "cslib/CSGlobal.h"
#include "cslib/CSStrUtil.h"
#include "cslib/CSLog.h"
#include "cslib/CSPath.h"
#include "cslib/CSDirectory.h"

#include "ha_pbms.h"
//#include <plugin.h>

#include "ms_mysql.h"
#include "Database_ms.h"
#include "OpenTable_ms.h"
#include "Util_ms.h"
#include "Discover_ms.h"
#include "SysTab_util.h"

#include "SysTab_cloud.h"

DT_FIELD_INFO pbms_cloud_info[]=
{
	{"Id",			NULL,	NULL, MYSQL_TYPE_LONG,		NULL,			NOT_NULL_FLAG,	"The Cloud storage reference ID"},
	{"Server",		1024,	NULL, MYSQL_TYPE_VARCHAR,	&UTF8_CHARSET,	NOT_NULL_FLAG,	"S3 server name"},
	{"Bucket",		124,	NULL, MYSQL_TYPE_VARCHAR,	&UTF8_CHARSET,	NOT_NULL_FLAG,	"S3 bucket name"},
	{"PublicKey",	124,	NULL, MYSQL_TYPE_VARCHAR,	&UTF8_CHARSET,	NOT_NULL_FLAG,	"S3 public key"},
	{"PrivateKey",	124,	NULL, MYSQL_TYPE_VARCHAR,	&UTF8_CHARSET,	NOT_NULL_FLAG,	"S3 private key"},
	{NULL,NULL, NULL, MYSQL_TYPE_STRING,NULL, 0, NULL}
};

DT_KEY_INFO pbms_cloud_keys[]=
{
	{"pbms_cloud_pk", PRI_KEY_FLAG, {"Id", NULL}},
	{NULL, 0, {NULL}}
};

#define MIN_CLOUD_TABLE_SIZE 4

//----------------------------
void MSCloudTable::startUp()
{
	MSCloudInfo::startUp();
}

//----------------------------
void MSCloudTable::shutDown()
{
	MSCloudInfo::shutDown();
}

//----------------------------
void MSCloudTable::loadTable(MSDatabase *db)
{

	enter_();
	
	push_(db);
	lock_(MSCloudInfo::gCloudInfo);
	
	if (MSCloudInfo::gMaxInfoRef == 0) {
		CSPath	*path;
		path = getSysFile(getPBMSPath(RETAIN(db->myDatabasePath)), CLOUD_TABLE_NAME, MIN_CLOUD_TABLE_SIZE);
		push_(path);

		if (path->exists()) {
			CSFile		*file;
			SysTabRec	*cloudData;
			const char	*server, *bucket, *pubKey, *privKey;
			uint32_t		info_id;
			MSCloudInfo	*info;
			size_t		size;
			
			new_(cloudData, SysTabRec("pbms", CLOUD_TABLE_NAME".dat", CLOUD_TABLE_NAME));
			push_(cloudData);

			file = path->openFile(CSFile::READONLY);
			push_(file);
			size = file->getEOF();
			cloudData->setLength(size);
			file->read(cloudData->getBuffer(0), 0, size, size);
			release_(file);
			
			cloudData->firstRecord();
			MSCloudInfo::gMaxInfoRef = cloudData->getInt4Field();
			
			if (! cloudData->isValidRecord()) 
				MSCloudInfo::gMaxInfoRef = 1;
			
			while (cloudData->nextRecord()) {
				info_id = cloudData->getInt4Field();
				server = cloudData->getStringField();
				bucket = cloudData->getStringField();
				pubKey = cloudData->getStringField();
				privKey = cloudData->getStringField();
				
				if (cloudData->isValidRecord()) {
					if (info_id > MSCloudInfo::gMaxInfoRef) {
						char msg[80];
						snprintf(msg, 80, "Cloud info id (%"PRIu32") larger than expected (%"PRIu32")\n", info_id, MSCloudInfo::gMaxInfoRef);
						CSL.log(self, CSLog::Warning, "pbms "CLOUD_TABLE_NAME".dat :possible damaged file or record. ");
						CSL.log(self, CSLog::Warning, msg);
						MSCloudInfo::gMaxInfoRef = info_id +1;
					}
					if ( MSCloudInfo::gCloudInfo->get(info_id)) {
						char msg[80];
						snprintf(msg, 80, "Duplicate Cloud info id (%"PRIu32") being ignored\n", info_id);
						CSL.log(self, CSLog::Warning, "pbms "CLOUD_TABLE_NAME".dat :possible damaged file or record. ");
						CSL.log(self, CSLog::Warning, msg);
					} else {
						new_(info, MSCloudInfo(	info_id, server, bucket, pubKey, privKey));
						MSCloudInfo::gCloudInfo->set(info_id, info);
					}
				}
			}
			release_(cloudData); cloudData = NULL;
			
		} else
			MSCloudInfo::gMaxInfoRef = 1;
		
		release_(path);
		
	}
	unlock_(MSCloudInfo::gCloudInfo);

	release_(db);

	exit_();
}

void MSCloudTable::saveTable(MSDatabase *db)
{
	SysTabRec		*cloudData;
	MSCloudInfo		*info;
	enter_();
	
	push_(db);
	
	new_(cloudData, SysTabRec("pbms", CLOUD_TABLE_NAME".dat", CLOUD_TABLE_NAME));
	push_(cloudData);
	
	// Build the table records
	cloudData->clear();
	lock_(MSCloudInfo::gCloudInfo);
	
	cloudData->beginRecord();	
	cloudData->setInt4Field(MSCloudInfo::gMaxInfoRef);
	cloudData->endRecord();	
	for  (int i = 0;(info = (MSCloudInfo*) MSCloudInfo::gCloudInfo->itemAt(i)); i++) { // info is not referenced.
		
		cloudData->beginRecord();	
		cloudData->setInt4Field(info->getCloudRefId());
		cloudData->setStringField(info->getServer());
		cloudData->setStringField(info->getBucket());
		cloudData->setStringField(info->getPublicKey());
		cloudData->setStringField(info->getPrivateKey());
		cloudData->endRecord();			
	}
	unlock_(MSCloudInfo::gCloudInfo);

	restoreTable(RETAIN(db), cloudData->getBuffer(0), cloudData->length(), false);
	
	release_(cloudData);
	release_(db);
	exit_();
}


MSCloudTable::MSCloudTable(MSSystemTableShare *share, TABLE *table):
MSOpenSystemTable(share, table),
iCloudIndex(0)
{
}

MSCloudTable::~MSCloudTable()
{
	//unuse();
}

void MSCloudTable::use()
{
	MSCloudInfo::gCloudInfo->lock();
}

void MSCloudTable::unuse()
{
	MSCloudInfo::gCloudInfo->unlock();
	
}


void MSCloudTable::seqScanInit()
{
	iCloudIndex = 0;
}

#define MAX_PASSWORD ((int32_t)64)
bool MSCloudTable::seqScanNext(char *buf)
{
	char		passwd[MAX_PASSWORD +1];
	TABLE		*table = mySQLTable;
	Field		*curr_field;
	byte		*save;
	MY_BITMAP	*save_write_set;
	MSCloudInfo	*info;
	const char	*val;
	
	enter_();
	
	info = (MSCloudInfo	*) MSCloudInfo::gCloudInfo->itemAt(iCloudIndex++); // Object is not referenced.
	if (!info)
		return_(false);
	
	save_write_set = table->write_set;
	table->write_set = NULL;

	memset(buf, 0xFF, table->s->null_bytes);
 	for (Field **field=table->field ; *field ; field++) {
 		curr_field = *field;
		save = curr_field->ptr;
#if MYSQL_VERSION_ID < 50114
		curr_field->ptr = (byte *) buf + curr_field->offset();
#else
		curr_field->ptr = (byte *) buf + curr_field->offset(curr_field->table->record[0]);
#endif
		switch (curr_field->field_name[0]) {
			case 'I':
				ASSERT(strcmp(curr_field->field_name, "Id") == 0);
				curr_field->store(info->getCloudRefId(), true);
				break;

			case 'S':
				ASSERT(strcmp(curr_field->field_name, "Server") == 0);
				val = info->getServer();
				curr_field->store(val, strlen(val), &UTF8_CHARSET);
				setNotNullInRecord(curr_field, buf);
				break;

			case 'B': 
				ASSERT(strcmp(curr_field->field_name, "Bucket") == 0);
				val = info->getBucket();
				curr_field->store(val, strlen(val), &UTF8_CHARSET);
				setNotNullInRecord(curr_field, buf);
				break;

			case 'P': 
				if (curr_field->field_name[1] == 'u') {
					ASSERT(strcmp(curr_field->field_name, "PublicKey") == 0);
					val = info->getPublicKey();
				} else if (curr_field->field_name[1] == 'r') {
					ASSERT(strcmp(curr_field->field_name, "PrivateKey") == 0);
					val = info->getPrivateKey();
					
					int32_t i;
					for (i = 0; (i < MAX_PASSWORD) && (i < (int32_t)strlen(val)); i++) passwd[i] = '*';
					passwd[i] = 0;
					val = passwd;
				} else {
					ASSERT(false);
					break;
				}
				curr_field->store(val, strlen(val), &UTF8_CHARSET);
				setNotNullInRecord(curr_field, buf);
				break;
				
			default:
				ASSERT(false);
		}
		curr_field->ptr = save;
	}

	table->write_set = save_write_set;
	
	return_(true);
}

void MSCloudTable::seqScanPos(uint8_t *pos)
{
	int32_t index = iCloudIndex -1;
	if (index < 0)
		index = 0; // This is probably an error condition.
		
	mi_int4store(pos, index);
}

void MSCloudTable::seqScanRead(uint8_t *pos, char *buf)
{
	iCloudIndex = mi_uint4korr(pos);
	seqScanNext(buf);
}

void MSCloudTable::updateRow(char *old_data, char *new_data) 
{
	TABLE	*table = mySQLTable;	
	uint32_t n_id, o_id, o_indx, n_indx;
	const char *realPrivKey;
	String server_val, bucket_val, pubKey_val, privKey_val, *server = &server_val, *bucket = &bucket_val, *pubKey = &pubKey_val, *privKey = &privKey_val;
	String o_server_val, o_bucket_val, o_pubKey_val, o_privKey_val, *o_server = &o_server_val, *o_bucket = &o_bucket_val, *o_pubKey = &o_pubKey_val, *o_privKey = &o_privKey_val;
	MSCloudInfo *info;

	enter_();
	
	GET_INT_FIELD(n_id, 0, table, new_data);		
	GET_STR_FIELD(server, 1, table, new_data);
	GET_STR_FIELD(bucket, 2, table, new_data);
	GET_STR_FIELD(pubKey, 3, table, new_data);
	GET_STR_FIELD(privKey, 4, table, new_data);
	
	GET_INT_FIELD(o_id, 0, table, old_data);
	GET_STR_FIELD(o_server, 1, table, old_data);
	GET_STR_FIELD(o_bucket, 2, table, old_data);
	GET_STR_FIELD(o_pubKey, 3, table, old_data);
	GET_STR_FIELD(o_privKey, 4, table, old_data);
	
	// The cloud ID must be unique
	if ((o_id !=  n_id) && MSCloudInfo::gCloudInfo->get(n_id)) {
		CSException::throwException(CS_CONTEXT, MS_ERR_DUPLICATE, "Attempt to update a row with a duplicate key in the "CLOUD_TABLE_NAME" table.");
	}

	// The private key is masked when returned to the caller, so
	// unless the caller has updated it we need to get the real 
	// private key from the old record.
	if (strcmp(privKey->c_ptr(), o_privKey->c_ptr()))
		realPrivKey = privKey->c_ptr();
	else {
		info = (MSCloudInfo*) MSCloudInfo::gCloudInfo->get(o_id); // unreference pointer
		realPrivKey = info->getPrivateKey();
	}
	
	new_(info, MSCloudInfo(	n_id, server->c_ptr(), bucket->c_ptr(), pubKey->c_ptr(), realPrivKey));
	push_(info);
	
	o_indx = MSCloudInfo::gCloudInfo->getIndex(o_id);

	MSCloudInfo::gCloudInfo->remove(o_id);
	pop_(info);
	MSCloudInfo::gCloudInfo->set(n_id, info);
	n_indx = MSCloudInfo::gCloudInfo->getIndex(n_id);
	
	// Adjust the current position in the array if required.
	if (o_indx < n_indx )
		iCloudIndex--;
		
	saveTable(RETAIN(myShare->mySysDatabase));
	exit_();
}

void MSCloudTable::insertRow(char *data) 
{
	TABLE	*table = mySQLTable;	
	uint32_t ref_id;
	String server_val, bucket_val, pubKey_val, privKey_val, *server = &server_val, *bucket = &bucket_val, *pubKey = &pubKey_val, *privKey = &privKey_val;
	MSCloudInfo *info;

	enter_();
	
	GET_INT_FIELD(ref_id, 0, table, data);
		
	// The cloud ID must be unique
	if (ref_id && MSCloudInfo::gCloudInfo->get(ref_id)) {
		CSException::throwException(CS_CONTEXT, MS_ERR_DUPLICATE, "Attempt to insert a row with a duplicate key in the "CLOUD_TABLE_NAME" table.");
	}
	
	GET_STR_FIELD(server, 1, table, data);
	GET_STR_FIELD(bucket, 2, table, data);
	GET_STR_FIELD(pubKey, 3, table, data);
	GET_STR_FIELD(privKey, 4, table, data);
	
	if (ref_id == 0)
		ref_id = MSCloudInfo::gMaxInfoRef++;
	else if (ref_id >= MSCloudInfo::gMaxInfoRef)
		MSCloudInfo::gMaxInfoRef = ref_id +1;
		
	new_(info, MSCloudInfo(	ref_id, server->c_ptr(), bucket->c_ptr(), pubKey->c_ptr(), privKey->c_ptr()));
	MSCloudInfo::gCloudInfo->set(ref_id, info);
	
	saveTable(RETAIN(myShare->mySysDatabase));
	exit_();
}

void MSCloudTable::deleteRow(char *data) 
{
	TABLE	*table = mySQLTable;	
	uint32_t ref_id, indx;

	enter_();
	
	GET_INT_FIELD(ref_id, 0, table, data);
		
	// Adjust the current position in the array if required.
	indx = MSCloudInfo::gCloudInfo->getIndex(ref_id);
	if (indx <= iCloudIndex)
		iCloudIndex--;

	MSCloudInfo::gCloudInfo->remove(ref_id);
	saveTable(RETAIN(myShare->mySysDatabase));
	exit_();
}

void MSCloudTable::transferTable(MSDatabase *to_db, MSDatabase *from_db)
{
	CSPath	*path;
	enter_();
	
	push_(from_db);
	push_(to_db);
	
	path = CSPath::newPath(getPBMSPath(RETAIN(from_db->myDatabasePath)), CLOUD_TABLE_NAME".dat");
	push_(path);
	if (path->exists()) {
		CSPath	*bu_path;
		bu_path = CSPath::newPath(getPBMSPath(RETAIN(to_db->myDatabasePath)), CLOUD_TABLE_NAME".dat");
		path->copyTo(bu_path, true);
	}
	
	release_(path);
	release_(to_db);
	release_(from_db);
	
	exit_();
}

CSStringBuffer *MSCloudTable::dumpTable(MSDatabase *db)
{

	CSPath			*path;
	CSStringBuffer	*dump;

	enter_();
	
	push_(db);
	path = getSysFile(getPBMSPath(RETAIN(db->myDatabasePath)), CLOUD_TABLE_NAME, MIN_CLOUD_TABLE_SIZE);
	release_(db);
	
	push_(path);
	new_(dump, CSStringBuffer(20));
	push_(dump);

	if (path->exists()) {
		CSFile	*file;
		size_t	size;
		
		file = path->openFile(CSFile::READONLY);
		push_(file);
		
		size = file->getEOF();
		dump->setLength(size);
		file->read(dump->getBuffer(0), 0, size, size);
		release_(file);
	}
	
	pop_(dump);
	release_(path);
	return_(dump);
}

void MSCloudTable::restoreTable(MSDatabase *db, const char *data, size_t size, bool reload)
{
	CSPath	*path;
	CSFile	*file;

	enter_();
	
	push_(db);
	path = getSysFile(getPBMSPath(RETAIN(db->myDatabasePath)), CLOUD_TABLE_NAME, MIN_CLOUD_TABLE_SIZE);
	push_(path);
	
	file = path->openFile(CSFile::CREATE | CSFile::TRUNCATE);
	push_(file);
	
	file->write(data, 0, size);
	file->close();
	release_(file);
	
	release_(path);
	
	pop_(db);
	if (reload)
		loadTable(db);
	else
		db->release();
		
	exit_();
}

void MSCloudTable::removeTable(CSString *db_path)
{
	CSPath	*path;
	char pbms_path[PATH_MAX];
	
	enter_();
	
	push_(db_path);	
	cs_strcpy(PATH_MAX, pbms_path, db_path->getCString());
	release_(db_path);
	
	if (strcmp(cs_last_name_of_path(pbms_path), "pbms")  != 0)
		exit_();
		
	cs_remove_last_name_of_path(pbms_path);

	path = getSysFile(CSString::newString(pbms_path), CLOUD_TABLE_NAME, MIN_CLOUD_TABLE_SIZE);
	push_(path);
	
	if (path->exists())
		path->removeFile();
	release_(path);
	
	exit_();
}

