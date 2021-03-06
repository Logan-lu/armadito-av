/***

Copyright (C) 2015, 2016 Teclib'

This file is part of Armadito core.

Armadito core is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Armadito core is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Armadito core.  If not, see <http://www.gnu.org/licenses/>.

***/

#include <libarmadito.h>
#include "os/dir.h"
#include "quarantine.h"
#include "os\string.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <json.h>


// TODO :: alert built-in module
#define ALERT_DIR "Alerts"

struct quarantine_data {
  char *quarantine_dir;
  int enable;
};

// Windows version.

/*
	This function returns the complete path of a location given in parameter
	according to the installation path.
*/
char * GetLocationCompletepath(char * specialDir) {

	char * dirpath = NULL;
	char * completePath = NULL;
	char filepath[MAX_PATH];	
	char * ptr = NULL;
	int dir_len = 0, len= 0;
	int ret = 0;
	
	__try {

		if (!GetModuleFileNameA(NULL, (LPSTR)&filepath, MAX_PATH)) {	
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: GetLocationCompletepath :: GetModuleFilename failed :: GLE = %d\n",GetLastError());
			return NULL;
		}

		// Remove the module filename from the complete file path
		ptr = strrchr(filepath,'\\');
		if (ptr == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: GetLocationCompletepath :: No backslash found in the path\n");
			return NULL;
		}

		// calc the dir buffer length.
		dir_len = (int)(ptr - filepath);
		dirpath = (char*)(calloc(dir_len+1,sizeof(char)));
		dirpath[dir_len] = '\0';

		memcpy_s(dirpath, dir_len, filepath, dir_len);
		//printf("[+] Debug :: GetLocationCompletepath :: dirpath = %s\n",dirpath);

		len = dir_len + strnlen(specialDir, MAX_PATH) + 2;
		
		completePath = (char*)calloc(len+1,sizeof(char));
		completePath[len] = '\0';

		strncat_s(completePath, len, dirpath, dir_len);
		strncat_s(completePath, len, "\\", 1);
		strncat_s(completePath, len, specialDir, strnlen(specialDir, MAX_PATH));		

		//printf("[+] Debug :: GetLocationCompletepath :: completePath = %s\n",completePath);

		
	}
	__finally {

		if (dirpath != NULL) {
			free(dirpath);
			dirpath = NULL;
		}

	}	

	return completePath;
}

char * GetTimestampString( ) {

	char * timestamp = NULL;
	SYSTEMTIME st = {0};
	int len = 0;

	// get the current system time
	GetSystemTime(&st);

	len = 15; // year + month + day + hour + minute + second
	timestamp = (char*)calloc(len+1,sizeof(char));
	timestamp[len] = '\0';
	sprintf_s(timestamp,len, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	return timestamp;
}

char * BuildLocationFilePath(char * filepath, char * specialDir) {

	char * qPath = NULL;
	int len = 0;
	char * filename = NULL;
	char * timestamp  = NULL;
	char * location_dir = NULL;


	if (filepath == NULL || specialDir == NULL) {
		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," BuildLocationFilePath :: Invalid parameter!\n");
		return NULL;
	}

	__try {

		location_dir = GetLocationCompletepath(specialDir);
		if (location_dir == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," BuildLocationFilePath :: Get Location complete path failed\n");			
			return NULL;
		}


		// Get the file name from the complete file path
		filename = strrchr(filepath,'\\');
		if (filename == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," BuildQuarantineFilePath!strrchr() failed :: backslash not found in the path :: %s.\n",filepath);			
			return NULL;
		}

		// get the current system time in format // year + month + day + hour + minute + second
		timestamp = GetTimestampString( );				

		// printf("[i] Debug :: BuildQuarantineFilePath :: filename = %s\n", filename);
		// printf("[i] Debug :: BuildQuarantineFilePath :: timestamp len = %d\n",strnlen_s(timestamp,MAX_PATH) );
		len = 0;
		len = strnlen_s(location_dir, MAX_PATH) + strnlen_s(filepath,MAX_PATH) + strnlen_s(timestamp,MAX_PATH) +3;
		//printf("[i] Debug :: BuildQuarantineFilePath :: len = %d\n", len);


		qPath = (char*)calloc(len + 1, sizeof(char));
		qPath[len] = '\0';

		strncat_s(qPath, len, location_dir, strnlen_s(location_dir, MAX_PATH));
		//printf("[i] Debug :: BuildQuarantineFilePath :: path = %s\n", qPath);		
		strncat_s(qPath, len, filename, strnlen_s(filename,MAX_PATH));
		strncat_s(qPath, len, "_", 1);
		strncat_s(qPath, len, timestamp, strnlen_s(timestamp,MAX_PATH));

		printf("[i] Debug :: BuildLocationFilePath :: path = %s\n", qPath);
		
	}
	__finally {

		if (timestamp != NULL) {
			free(timestamp);
			timestamp = NULL;
		}

		if (location_dir != NULL) {
			free(location_dir);
			location_dir = NULL;
		}

	}

	return qPath;

}

char * GetFilenameFromPath(char * path) {

	char * filename = NULL;

	if (path == NULL) {
		return NULL;
	}

	__try {

		filename = strrchr(path,'\\');
		if (filename == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," GetFilenameFromPath!strrchr() failed :: backslash not found in the path :: %s.\n",path);			
			return NULL;
		}

	}
	__finally {

	}

	return filename;
}

int WriteQuarantineInfoFile(char * oldfilepath, char * quarantinePath, struct a6o_report * report) {

	int ret = 0;
	char * info_path = NULL;
	char * alert_path = NULL;
	int len = 0;
	char * alert_tmp = NULL;
	char * content = NULL;
	char * timestamp = NULL;
	HANDLE fh = INVALID_HANDLE_VALUE;
	int written = 0;
	struct json_object *jobj = NULL;
	

	if (oldfilepath == NULL || quarantinePath == NULL ) {
		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: WriteQuarantineInfoFile :: invalid parameter\n");
		return -1;
	}

	__try {

		// build information file path for quarantine.
		len = strnlen_s(quarantinePath ,MAX_PATH) + strnlen_s(".info" ,MAX_PATH) +1;
		info_path = (char*)calloc(len + 1, sizeof(char));
		info_path[len] = '\0';

		strncat_s(info_path, len, quarantinePath, strnlen_s(quarantinePath ,MAX_PATH));
		strncat_s(info_path, len, ".info", strnlen_s(".info" ,MAX_PATH));

		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_INFO,"[+] Debug :: WriteQuarantineInfoFile :: info file path = %s\n",info_path);

		// build information file path for quarantine.
		alert_tmp = BuildLocationFilePath(oldfilepath, ALERT_DIR);
		len = 0;
		len = strnlen_s(alert_tmp ,MAX_PATH) + strnlen_s(".info" ,MAX_PATH) +1;
		alert_path = (char*)calloc(len + 1, sizeof(char));
		alert_path[len] = '\0';

		strncat_s(alert_path, len, alert_tmp, strnlen_s(alert_tmp ,MAX_PATH));
		strncat_s(alert_path, len, ".info", strnlen_s(".info" ,MAX_PATH));

		
		printf("[+] Debug :: WriteQuarantineInfoFile :: alert file path = %s\n",alert_path);


		// create content. Ex: {"date":"20160224142724","path":"C:\\Quarantine\\malware.txt","module":"clamav"}

		timestamp = GetTimestampString();

		jobj = json_object_new_object();
		json_object_object_add(jobj, "date", json_object_new_string(timestamp));

		json_object_object_add(jobj, "fname", json_object_new_string(GetFilenameFromPath(quarantinePath)));

		// TODO :: add file checksum..
		//json_object_object_add(jobj, "checksum", json_object_new_string(GetFilenameFromPath(quarantinePath)));
		
		json_object_object_add(jobj, "path", json_object_new_string(oldfilepath));

		if (report != NULL && report->mod_name != NULL) {
			json_object_object_add(jobj, "module", json_object_new_string(report->mod_name));
		}
		else {
			json_object_object_add(jobj, "module", json_object_new_string("black_list"));
		}

		if (report != NULL && report->mod_report != NULL) {
			json_object_object_add(jobj, "desc", json_object_new_string(report->mod_report));
		}
		else {
			json_object_object_add(jobj, "desc", json_object_new_string("uh_malware"));
		}
		
		content = json_object_to_json_string(jobj);
		if (content == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: WriteQuarantineInfoFile :: can't build info content\n");
			ret = -2;
			__leave;
		}
		//printf("[+] Debug :: json content = %s \n",content);
		
		
		// write in file.
		/*if ((ret = json_object_to_file(info_path, jobj)) != 0) {
			printf("[-] Error :: WriteQuarantineInfoFile :: writing information file failed !\n",ret);
			__leave;
		}*/

		fh = CreateFile(info_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE) {
			ret = -3;
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: WriteQuarantineInfoFile :: can't open the info file [%s] :: GLE = %d\n",info_path,GetLastError());
			__leave;
		}

		len = strnlen(content,2048);

		if (WriteFile(fh, content, len, &written, NULL) == FALSE) {
			ret = -3;
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: WriteQuarantineInfoFile :: write failed :: GLE = %d\n",GetLastError());
			__leave;
		}

		if (fh != INVALID_HANDLE_VALUE) {
			CloseHandle(fh);
			fh = INVALID_HANDLE_VALUE;
		}

		// copy info file in alert
		if (CopyFile(info_path,alert_path,TRUE)  == FALSE){
			//ret = -4;
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: WriteQuarantineInfoFile :: Copy info file in alert dir failed :: GLE = %d\n",GetLastError());
			//__leave;
		}

	}
	__finally {

		if (info_path != NULL) {
			free(info_path);
			info_path = NULL;
		}

		if (alert_path != NULL) {
			free(alert_path);
			alert_path = NULL;
		}

		if (timestamp != NULL) {
			free(timestamp);
			timestamp = NULL;
		}

		if (jobj != NULL) {
			json_object_put(jobj);
		}
		
		if (fh != INVALID_HANDLE_VALUE) {
			CloseHandle(fh);
			fh = INVALID_HANDLE_VALUE;
		}
		
	}

	return  ret;

}

int CreateAvDirectory( char * dir) {

	int ret = 0;
	char * completePath = NULL;

	if (dir == NULL) {
		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," CreateAvDirectory :: Invalid parameter!\n");
		return -1;
	}

	__try {

		completePath = GetLocationCompletepath(dir);
		if (completePath == NULL) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," CreateAvDirectory :: Get Complete path failed!\n");
			ret = -2;
			__leave;
		}


		if (PathFileExistsA(completePath) == 1) {
			ret = 1;
			__leave;
		}

		if (!CreateDirectoryA(completePath, NULL)) {
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," CreateAvDirectory :: Directory creation failed! :: GLE = %d\n",GetLastError());
			ret = -3;
			__leave;
		}

	}
	__finally {

		if (completePath != NULL) {
			free(completePath);
			completePath = NULL;
		}

	}

	return ret;
}


static int quarantine_do(struct quarantine_data *qu_data, const char *path, struct a6o_report * report)
{
	int ret = 0;
	char * quarantineFilepath = NULL;
	char * quarantineDir = NULL;
	HANDLE fh = INVALID_HANDLE_VALUE;

	//printf("[+] Debug :: quarantine_do path= %s -----------------\n",path);	


	if (path == NULL) {		
		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," quarantine_do :: Invalid file path!\n");
		return -1;
	}

	__try {

		// build quarantine file path.
		quarantineFilepath = BuildLocationFilePath(path,qu_data->quarantine_dir);
		if (quarantineFilepath == NULL) {			
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," Build Quarantine FilePath failed ! :: [%s]\n",path);
			ret = -2;
			__leave;
		}

		// create quarantine folder if needed.
		if (CreateAvDirectory(qu_data->quarantine_dir) < 0) {			
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," Quarantine folder creation failed!\n");
			ret = -3;
			__leave;
		}		

		// try to open the file.
		/*fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, NULL);
		if (fh == INVALID_HANDLE_VALUE) {
			printf("[-] Error :: quarantine_do :: Opening the file [%s] failed! :: error = %d\n",path,GetLastError());
			ret = -5;
			__leave;
		}	
		printf("[+] Debug :: quarantine_do :: file [%s] opened successfully !!\n",path);
		CloseHandle(fh);
		*/

		// Move file to quarantine directory.		
		if (MoveFileEx(path,quarantineFilepath,MOVEFILE_REPLACE_EXISTING|MOVEFILE_FAIL_IF_NOT_TRACKABLE|MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH) == FALSE){			
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," Move file [%s] to quarantine folder failed ! :: GLE =%d\n",path,GetLastError());
			ret = -4;
			__leave;
		}

		// Write quarantine .info file
		if (WriteQuarantineInfoFile(path, quarantineFilepath, report) != 0) {
			ret = -5;			
			a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_ERROR," Write Quarantine Information File failed! :: [%s] :: [%s]\n",path, quarantineFilepath);
			__leave;
		}

		printf("[+] Debug :: File [%s] moved to quarantine folder successfully !\n", path);	
		a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_INFO," File [%s] moved to quarantine folder successfully !\n",path );
		a6o_notify(NOTIF_WARNING,"File [%s] moved to quarantine!\n");


	}
	__finally {

		if (quarantineFilepath != NULL) {
			free(quarantineFilepath);
			quarantineFilepath = NULL;
		}		

	}	


	return ret;
}

void quarantine_callback(struct a6o_report *report, void *callback_data)
{
  struct quarantine_data *qu_data = (struct quarantine_data *)callback_data;
  
  //printf("[+] Debug :: quarantine_callback :: enable = %d :: dir = %s :: status = %d \n", qu_data->enable, qu_data->quarantine_dir, report->status);
  //a6o_log(ARMADITO_LOG_LIB,ARMADITO_LOG_LEVEL_INFO, "[+] Debug :: quarantine_callback :: enable = %d :: dir = %s :: status = %d\n", qu_data->enable, qu_data->quarantine_dir, report->status);

  if (!qu_data->enable)
    return;

  switch(report->status) {
  case ARMADITO_UNDECIDED:
  case ARMADITO_CLEAN:
  case ARMADITO_UNKNOWN_FILE_TYPE:
  case ARMADITO_EINVAL:
  case ARMADITO_IERROR:
  case ARMADITO_SUSPICIOUS:
  case ARMADITO_WHITE_LISTED:
    return;
  }

  if (quarantine_do(qu_data, report->path, report) == 0)
    report->action |= ARMADITO_ACTION_QUARANTINE;
}

static enum a6o_mod_status quarantine_init(struct a6o_module *module)
{
  struct quarantine_data *qu_data = calloc(1, sizeof(struct quarantine_data));

  qu_data->quarantine_dir = NULL;
  qu_data->enable = 0;

  module->data = qu_data;

  return ARMADITO_MOD_OK;


}

static enum a6o_mod_status quarantine_conf_quarantine_dir(struct a6o_module *module, const char *key, struct a6o_conf_value *value)
{
	struct quarantine_data *qu_data = (struct quarantine_data *)module->data;

	qu_data->quarantine_dir = os_strdup(a6o_conf_value_get_string(value));

	return ARMADITO_MOD_OK;

}

static enum a6o_mod_status quarantine_conf_enable(struct a6o_module *module, const char *key, struct a6o_conf_value *value)
{
	struct quarantine_data *qu_data = (struct quarantine_data *)module->data;

	qu_data->enable = a6o_conf_value_get_int(value);

	return ARMADITO_MOD_OK;
}


struct a6o_conf_entry quarantine_conf_table[] = {
	{ "enable", CONF_TYPE_INT, quarantine_conf_enable},
	{ "quarantine-dir", CONF_TYPE_STRING, quarantine_conf_quarantine_dir},
	{ NULL, 0, NULL},
};

struct a6o_module quarantine_module = {
	.init_fun = quarantine_init,
	.conf_table = quarantine_conf_table,
	.post_init_fun = NULL,
	.scan_fun = NULL,
	.close_fun = NULL,
	.supported_mime_types = NULL,
	.name = "quarantine",
};
