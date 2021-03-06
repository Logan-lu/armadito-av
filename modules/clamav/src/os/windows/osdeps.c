/***

Copyright (C) 2015, 2016 Teclib'

This file is part of Armadito module clamav.

Armadito module clamav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Armadito module clamav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Armadito module clamav.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "..\osdeps.h"
#include <libarmadito.h>

#define MAX_PATH 260


char * get_db_module_path(char * filename, char * module) {

	char * completePath = NULL;
	char * dbdir = NULL;
	int dbdir_len = 0;
	int len = 0;
	int off = 0;
	char path_sep = '\0';

	if (filename == NULL || module == NULL) {
		a6o_log(ARMADITO_LOG_MODULE, ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: get_db_module_path :: Invalids parameters\n");
		return NULL;
	}

	dbdir = a6o_std_path(BASES_LOCATION);
	if (dbdir == NULL) {
		a6o_log(ARMADITO_LOG_MODULE, ARMADITO_LOG_LEVEL_ERROR,"[-] Error :: get_db_module_path :: Can't get database directory!\n");
		return NULL;
	}

	// get path separator.
	path_sep = a6o_path_sep( );
	
	len = strnlen_s(dbdir,MAX_PATH) + strnlen_s(module,MAX_PATH) + strnlen_s(filename,MAX_PATH) + 2;	
	completePath = (char*)calloc(len+1,sizeof(char));
	completePath[len] = '\0';

	memcpy_s(completePath, len, dbdir, strnlen_s(dbdir, MAX_PATH));	
	off += strnlen_s(dbdir, MAX_PATH);
	completePath[off] = path_sep;
	off++;	
	memcpy_s(completePath + off, len, module, strnlen_s(module, MAX_PATH));
	off += strnlen_s(module, MAX_PATH);
	completePath[off] = path_sep;
	off++;
	memcpy_s(completePath + off, len, filename, strnlen_s(filename, MAX_PATH));

	//printf("[+] Debug :: get_db_module_path :: completePath = %s\n", completePath);

	return completePath;

}
