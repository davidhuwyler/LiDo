/*  Glue functions for the minIni library, based on the FatFs and Petit-FatFs
 *  libraries, see http://elm-chan.org/fsw/ff/00index_e.html
 *
 *  By CompuPhase, 2008-2012
 *  This "glue file" is in the public domain. It is distributed without
 *  warranties or conditions of any kind, either express or implied.
 *
 *  (The FatFs and Petit-FatFs libraries are copyright by ChaN and licensed at
 *  its own terms.)
 */


#include "FileSystem.h"
#include "littleFS/lfs.h"
#include "minGlue-LittleFS.h"
#include <stddef.h>
#include <string.h>
#include "UTIL1.h"

int ini_rename(char *source, const char *dest)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	/* Function f_rename() does not allow drive letters in the destination file */
	char *drive = strchr(dest, ':');
	drive = (drive == NULL) ? (char*)dest : drive + 1;
	return (lfs_rename(FS_lfs, source, dest) == 0);
}

int ini_fileReadOpen(lfs_file_t *file, const char *name)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	return (lfs_file_open(FS_lfs, file, name, LFS_O_RDONLY)== 0);
}

int ini_fileWriteOpen(lfs_file_t *file, const char *name)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	return (lfs_file_open(FS_lfs, file, name, LFS_O_WRONLY | LFS_O_CREAT)== 0);
}

int ini_fileClose(lfs_file_t *file)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	return (lfs_file_close(FS_lfs, file) == 0);
}

int ini_fileRead( void *buffer, lfs_size_t size, lfs_file_t *file)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	int32_t res = lfs_file_read(FS_lfs, file, buffer, size);
	if(res >= 0)
	{
		return res;
	}
	return false;
}

int ini_fileWrite(void *buffer, lfs_file_t *file)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	int32_t res = lfs_file_write(FS_lfs, file, buffer ,UTIL1_strlen(buffer));
	if(res >= 0)
	{
		return res;
	}
	return false;
}

int ini_fileRemove(const char *filename)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	return (lfs_remove(FS_lfs, filename) == 0);
}

int ini_fileTell(lfs_file_t *file ,unsigned long* pos)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	*pos = lfs_file_tell(FS_lfs, file);
	return true;
}

int ini_fileSeek(lfs_file_t *file ,unsigned long* pos)
{
	lfs_t* FS_lfs = FS_GetFileSystem();
	lfs_file_seek(FS_lfs, file, *pos, LFS_SEEK_SET);
	return true;
}


