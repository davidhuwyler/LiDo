/*
 * FileSystem.c
 *
 *  Created on: Mar 1, 2019
 *      Author: dave
 *
 *  Source Ported from:
 *  https://github.com/ErichStyger/mcuoneclipse/blob/master/Examples/KDS/tinyK20/tinyK20_LittleFS_W25Q128/Sources/fs.c
 */

#include "FileSystem.h"
#include "SPIF.h"
#include "CLS1.h"
#include "TmDt1.h"
#include "littleFS/lfs.h"
#include "Shell.h"

#define FS_FILE_NAME_SIZE  32 /* Length of file name, used in buffers */

/* variables used by the file system */
static bool FS_isMounted = FALSE;
static lfs_t FS_lfs;
static SemaphoreHandle_t fileSystemAccessSema;

#define FILESYSTEM_READ_BUFFER_SIZE 256//256
#define FILESYSTEM_PROG_BUFFER_SIZE 256//256
#define FILESYSTEM_LOOKAHEAD_SIZE 128 //128

static int block_device_read(const struct lfs_config *c, lfs_block_t block,	lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint8_t res;
	res = SPIF_Read(block * c->block_size + off, buffer, size);
	if (res != ERR_OK)
	{
		return LFS_ERR_IO;
	}
	return LFS_ERR_OK;
}

int block_device_prog(const struct lfs_config *c, lfs_block_t block,lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint8_t res;
	res = SPIF_ProgramPage(block * c->block_size + off, buffer, size);
	if (res != ERR_OK)
	{
		return LFS_ERR_IO;
	}
	return LFS_ERR_OK;
}

int block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
	uint8_t res;
	res = SPIF_EraseSector4K(block * c->block_size);
	if (res != ERR_OK)
	{
		return LFS_ERR_IO;
	}
	return LFS_ERR_OK;
}

int block_device_sync(const struct lfs_config *c)
{
	return LFS_ERR_OK;
}

// configuration of the file system is provided by this struct
const struct lfs_config FS_cfg = {
		// block device operations
		.read = block_device_read,
		.prog = block_device_prog,
		.erase =block_device_erase,
		.sync = block_device_sync,
		// block device configuration
		.read_size = FILESYSTEM_READ_BUFFER_SIZE,
		.prog_size = FILESYSTEM_PROG_BUFFER_SIZE,
		.block_size = 4096,
		.block_count = 16384, /* 16384 * 4K = 64 MByte */
		.lookahead = 128,
};


/*-----------------------------------------------------------------------*/
/* Get a string from the file
 * (ported from FatFS function: f_gets())
/*-----------------------------------------------------------------------*/

char* FS_gets (
  char* buff,  /* Pointer to the string buffer to read */
  int len,      /* Size of string buffer (characters) */
  lfs_file_t* fp       /* Pointer to the file object */
)
{
  int n = 0;
  char c, *p = buff;
  byte s[2];
  uint32_t rc;
  while (n < len - 1) { /* Read characters until buffer gets filled */
    rc = lfs_file_read(&FS_lfs,fp,s,1);
    if (rc != 1) break;
    c = s[0];

    if (c == '\r') continue; /* Strip '\r' */
    *p++ = c;
    n++;
    if (c == '\n') break;   /* Break on EOL */
  }
  *p = 0;
  return n ? buff : 0;      /* When no data read (eof or error), return with error. */
}

/*-----------------------------------------------------------------------*/
/* Put a character to the file
 * (ported from FatFS)
/*-----------------------------------------------------------------------*/

typedef struct {
  lfs_file_t* fp ;
  int idx, nchr;
  byte buf[64];
} putbuff;


static
void putc_bfd (
  putbuff* pb,
  char c
)
{
  uint32_t bw;
  int32_t i;

  if (c == '\n') {  /* LF -> CRLF conversion */
    putc_bfd(pb, '\r');
  }

  i = pb->idx;  /* Buffer write index (-1:error) */
  if (i < 0) return;

  pb->buf[i++] = (byte)c;

  if (i >= (int)(sizeof pb->buf) - 3) { /* Write buffered characters to the file */
	bw = lfs_file_write(&FS_lfs,pb->fp, pb->buf,(uint32_t)i);
    i = (bw == (uint32_t)i) ? 0 : -1;
  }
  pb->idx = i;
  pb->nchr++;
}



int FS_putc (
  char c,  /* A character to be output */
  lfs_file_t* fp   /* Pointer to the file object */
)
{
  putbuff pb;
  uint32_t nw;
  pb.fp = fp;     /* Initialize output buffer */
  pb.nchr = pb.idx = 0;
  putc_bfd(&pb, c); /* Put a character */
  nw = lfs_file_write(&FS_lfs,pb.fp, pb.buf, (uint32_t)pb.idx);
  if (   pb.idx >= 0  /* Flush buffered characters to the file */
    && nw>=0
    && (uint32_t)pb.idx == nw) return pb.nchr;
  return -1;
}


/*-----------------------------------------------------------------------*/
/* Put a string to the file
 * (ported from FatFS function: f_puts())                                            */
/*-----------------------------------------------------------------------*/


int FS_puts (
  const char* str, /* Pointer to the string to be output */
  lfs_file_t* fp       /* Pointer to the file object */
)
{
  putbuff pb;
  uint32_t nw;

  pb.fp = fp;       /* Initialize output buffer */
  pb.nchr = pb.idx = 0;

  while (*str)      /* Put the string */
    putc_bfd(&pb, *str++);

  nw = lfs_file_write(&FS_lfs,pb.fp, pb.buf, (uint32_t)pb.idx);

  if (   pb.idx >= 0    /* Flush buffered characters to the file */
    && nw>=0
    && (uint32_t)pb.idx == nw) return pb.nchr;
  return -1;
}


uint8_t FS_Format(CLS1_ConstStdIOType *io)
{
	int res;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("File system is mounted, unmount it first.\r\n",io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (io != NULL)
		{
			CLS1_SendStr("Formatting ...", io->stdOut);
		}
		res = lfs_format(&FS_lfs, &FS_cfg);
		if (res == LFS_ERR_OK)
		{
			if (io != NULL)
			{
				CLS1_SendStr(" done.\r\n", io->stdOut);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
		}
		else
		{
			if (io != NULL)
			{
				CLS1_SendStr(" FAILED!\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
	}
	else
	{
		return ERR_BUSY;
	}
}

uint8_t FS_Mount(CLS1_ConstStdIOType *io)
{
	int res;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("File system is already mounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (io != NULL)
		{
			CLS1_SendStr("Mounting ...", io->stdOut);
		}
		res = lfs_mount(&FS_lfs, &FS_cfg);
		if (res == LFS_ERR_OK)
		{
			if (io != NULL)
			{
				CLS1_SendStr(" done.\r\n", io->stdOut);
			}
			FS_isMounted = TRUE;
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
		}
		else
		{
			if (io != NULL) {
				CLS1_SendStr(" FAILED!\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
	}
	else
	{
		return ERR_BUSY;
	}
}

uint8_t FS_Unmount(CLS1_ConstStdIOType *io)
{
	int res;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("File system is already unmounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (io != NULL)
		{
			CLS1_SendStr("Unmounting ...", io->stdOut);
		}
		res = lfs_unmount(&FS_lfs);
		if (res == LFS_ERR_OK)
		{
			if (io != NULL)
			{
				CLS1_SendStr(" done.\r\n", io->stdOut);
			}
			FS_isMounted = FALSE;
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
		}
		else
		{
			if (io != NULL)
			{
				CLS1_SendStr(" FAILED!\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
	}
	else
	{
		return ERR_BUSY;
	}

}

uint8_t FS_Dir(const char *path, CLS1_ConstStdIOType *io)
{
	int res;
	lfs_dir_t dir;
	struct lfs_info info;

	if (io == NULL)
	{
		return ERR_FAILED; /* listing a directory without an I/O channel does not make any sense */
	}

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			CLS1_SendStr("File system is not mounted, mount it first.\r\n",	io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (path == NULL)
		{
			path = "/"; /* default path */
		}
		res = lfs_dir_open(&FS_lfs, &dir, path);
		if (res != LFS_ERR_OK)
		{
			CLS1_SendStr("FAILED lfs_dir_open()!\r\n", io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		for (;;)
		{
			res = lfs_dir_read(&FS_lfs, &dir, &info);
			if (res < 0)
			{
				CLS1_SendStr("FAILED lfs_dir_read()!\r\n", io->stdErr);
				xSemaphoreGive(fileSystemAccessSema);
				return ERR_FAILED;
			}
			if (res == 0)
			{ /* no more files */
				break;
			}
			switch (info.type)
			{
			case LFS_TYPE_REG:
				CLS1_SendStr("reg ", io->stdOut);
				break;
			case LFS_TYPE_DIR:
				CLS1_SendStr("dir ", io->stdOut);
				break;
			default:
				CLS1_SendStr("?   ", io->stdOut);
				break;
			}
			static const char *prefixes[] = { "", "K", "M", "G" }; /* prefixes for kilo, mega and giga */
			for (int i = sizeof(prefixes) / sizeof(prefixes[0]) - 1; i >= 0; i--)
			{
				if (info.size >= (1 << 10 * i) - 1)
				{
					CLS1_printf("%*u%sB ", 4 - (i != 0), info.size >> 10 * i,
							prefixes[i]); /* \todo: remove printf */
					break;
				}
			} /* for */
			CLS1_SendStr(info.name, io->stdOut);
			CLS1_SendStr("\r\n", io->stdOut);
		} /* for */
		res = lfs_dir_close(&FS_lfs, &dir);
		if (res != LFS_ERR_OK) {
			CLS1_SendStr("FAILED lfs_dir_close()!\r\n", io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}

uint8_t FS_FileList(const char *path, CLS1_ConstStdIOType *io)
{
	int res;
	lfs_dir_t dir;
	struct lfs_info info;

	if (io == NULL)
	{
		return ERR_FAILED; /* listing a directory without an I/O channel does not make any sense */
	}

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			CLS1_SendStr("File system is not mounted, mount it first.\r\n",	io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (path == NULL)
		{
			path = "/"; /* default path */
		}
		res = lfs_dir_open(&FS_lfs, &dir, path);
		if (res != LFS_ERR_OK)
		{
			CLS1_SendStr("FAILED lfs_dir_open()!\r\n", io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		for (;;)
		{
			res = lfs_dir_read(&FS_lfs, &dir, &info);
			if (res < 0)
			{
				CLS1_SendStr("FAILED lfs_dir_read()!\r\n", io->stdErr);
				xSemaphoreGive(fileSystemAccessSema);
				return ERR_FAILED;
			}
			if (res == 0)
			{ /* no more files */
				break;
			}

			if(info.type == LFS_TYPE_REG)
			{
				CLS1_SendStr(info.name, io->stdOut);
				CLS1_SendStr("\r\n", io->stdOut);
			}
		} /* for */
		res = lfs_dir_close(&FS_lfs, &dir);
		if (res != LFS_ERR_OK) {
			CLS1_SendStr("FAILED lfs_dir_close()!\r\n", io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}

}


uint8_t FS_CopyFile(const char *srcPath, const char *dstPath,CLS1_ConstStdIOType *io)
{
	lfs_file_t fsrc, fdst;
	uint8_t res = ERR_OK;
	int result, nofBytesRead;
	uint8_t buffer[32]; /* copy buffer */

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: File system is not mounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}

		/* open source file */
		result = lfs_file_open(&FS_lfs, &fsrc, srcPath, LFS_O_RDONLY);
		if (result < 0)
		{
			CLS1_SendStr((const unsigned char*) "*** Failed opening source file!\r\n",io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		/* create destination file */
		result = lfs_file_open(&FS_lfs, &fdst, dstPath, LFS_O_WRONLY | LFS_O_CREAT);
		if (result < 0)
		{
			(void) lfs_file_close(&FS_lfs, &fsrc);
			CLS1_SendStr((const unsigned char*) "*** Failed opening destination file!\r\n",	io->stdErr);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		/* now copy source to destination */
		for (;;)
		{
			nofBytesRead = lfs_file_read(&FS_lfs, &fsrc, buffer, sizeof(buffer));
			if (nofBytesRead < 0)
			{
				CLS1_SendStr((const unsigned char*) "*** Failed reading source file!\r\n",io->stdErr);
				res = ERR_FAILED;
				break;
			}
			if (nofBytesRead == 0) { /* end of file */
				break;
			}
			result = lfs_file_write(&FS_lfs, &fdst, buffer, nofBytesRead);
			if (result < 0)
			{
				CLS1_SendStr((const unsigned char*) "*** Failed writing destination file!\r\n",	io->stdErr);
				res = ERR_FAILED;
				break;
			}
		} /* for */
		/* close all files */
		result = lfs_file_close(&FS_lfs, &fsrc);
		if (result < 0)
		{
			CLS1_SendStr((const unsigned char*) "*** Failed closing source file!\r\n",	io->stdErr);
			res = ERR_FAILED;
		}
		result = lfs_file_close(&FS_lfs, &fdst);
		if (result < 0)
		{
			CLS1_SendStr((const unsigned char*) "*** Failed closing destination file!\r\n",	io->stdErr);
			res = ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}


}

uint8_t FS_MoveFile(const char *srcPath, const char *dstPath,CLS1_ConstStdIOType *io)
{
	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: File system is not mounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		if (lfs_rename(&FS_lfs, srcPath, dstPath) < 0)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: failed renaming file or directory.\r\n",io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}

}


/*
 * Used to read out data from Files for SDEP communication
 */
uint8_t FS_ReadFile(const char *filePath, bool readFromBeginning, size_t nofBytes, CLS1_ConstStdIOType *io)
{
	static lfs_file_t file;
	static int32_t filePos;
	size_t fileSize;

	uint8_t buf[200];

	if( nofBytes > 200)
	{
		nofBytes = 200;
	}

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (lfs_file_open(&FS_lfs, &file, filePath, LFS_O_RDONLY) < 0)
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}

		if(readFromBeginning)
		{
			lfs_file_rewind(&FS_lfs,&file);
			filePos = 0;
		}
		else
		{
			lfs_file_seek(&FS_lfs,&file, filePos,LFS_SEEK_SET);
		}

		fileSize = lfs_file_size(&FS_lfs, &file);
		filePos = lfs_file_tell(&FS_lfs, &file);
		fileSize = fileSize - filePos;

		if (fileSize < 0)
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}

		if(fileSize > nofBytes)
		{
			if (lfs_file_read(&FS_lfs, &file, buf, nofBytes) < 0)
			{
				return ERR_FAILED;
			}
			lfs_file_close(&FS_lfs, &file);
			CLS1_SendData(buf,nofBytes,io->stdErr);
			filePos = filePos + nofBytes;
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
		}
		else
		{
			if (lfs_file_read(&FS_lfs, &file, buf, fileSize) < 0)
			{
				return ERR_FAILED;
			}
			lfs_file_close(&FS_lfs, &file);
			CLS1_SendData(buf,fileSize,io->stdErr);
			filePos = filePos + fileSize;
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_PARAM_SIZE; //EOF
		}

	}
	else
	{
		return ERR_BUSY;
	}
}


uint8_t FS_openLiDoSampleFile(lfs_file_t* file)
{
	DATEREC date;
	uint8_t fileNameBuf[20];
	TmDt1_GetDate(&date);
	fileNameBuf[0] = '\0';
	UTIL1_strcatNum16uFormatted(fileNameBuf, 15, date.Day, '0', 2);
	UTIL1_chcat(fileNameBuf, 15, '_');
	UTIL1_strcatNum16uFormatted(fileNameBuf, 15, date.Month, '0', 2);
	UTIL1_chcat(fileNameBuf, 15, '_');
	UTIL1_strcatNum16u(fileNameBuf, 15, (uint16_t)date.Year);
	UTIL1_strcat(fileNameBuf, 15, ".txt");

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (lfs_file_open(&FS_lfs, file, fileNameBuf, LFS_O_WRONLY | LFS_O_CREAT| LFS_O_APPEND) < 0)
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}

uint8_t FS_closeLiDoSampleFile(lfs_file_t* file)
{
	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if(lfs_file_close(&FS_lfs, file) ==0)
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
		}
		else
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
	}
	else
	{
		return ERR_BUSY;
	}
}

/*
 * Write a LiDo Sample to the file with the name of the current date
 */
uint8_t FS_writeLiDoSample(liDoSample_t *sample,lfs_file_t* file)
{
	uint8_t lidoSampleBuf[LIDO_SAMPLE_SIZE];

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		lidoSampleBuf[0] = '@';		//SampleMarker
		lidoSampleBuf[1] = (uint8_t)sample->unixTimeStamp;
		lidoSampleBuf[2] = (uint8_t)(sample->unixTimeStamp>>8);
		lidoSampleBuf[3] = (uint8_t)(sample->unixTimeStamp>>16);
		lidoSampleBuf[4] = (uint8_t)(sample->unixTimeStamp>>24);
		lidoSampleBuf[5] = (uint8_t)sample->lightChannelX;
		lidoSampleBuf[6] = (uint8_t)(sample->lightChannelX>>8);
		lidoSampleBuf[7] = (uint8_t)sample->lightChannelY;
		lidoSampleBuf[8] = (uint8_t)(sample->lightChannelY>>8);
		lidoSampleBuf[9] = (uint8_t)sample->lightChannelZ;
		lidoSampleBuf[10] = (uint8_t)(sample->lightChannelZ>>8);
		lidoSampleBuf[11] = (uint8_t)sample->lightChannelIR;
		lidoSampleBuf[12] = (uint8_t)(sample->lightChannelIR>>8);
		lidoSampleBuf[13] = (uint8_t)sample->lightChannelB440;
		lidoSampleBuf[14] = (uint8_t)(sample->lightChannelB440>>8);
		lidoSampleBuf[15] = (uint8_t)sample->lightChannelB490;
		lidoSampleBuf[16] = (uint8_t)(sample->lightChannelB490>>8);
		lidoSampleBuf[17] = (uint8_t)sample->accelX;
		lidoSampleBuf[18] = (uint8_t)sample->accelY;
		lidoSampleBuf[19] = (uint8_t)sample->accelZ;
		lidoSampleBuf[20] = (uint8_t)sample->temp;
		lidoSampleBuf[21] = (uint8_t)sample->crc;

		if (lfs_file_write(&FS_lfs, file, lidoSampleBuf,LIDO_SAMPLE_SIZE) < 0)
		{
			lfs_file_close(&FS_lfs, file);
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}

static uint8_t readFromFile(void *hndl, uint32_t addr, uint8_t *buf,size_t bufSize)
{
	lfs_file_t *fp;

	fp = (lfs_file_t*) hndl;
	if (lfs_file_read(&FS_lfs, fp, buf, bufSize) < 0)
	{
		return ERR_FAILED;
	}
	return ERR_OK;
}

uint8_t FS_PrintHexFile(const char *filePath, CLS1_ConstStdIOType *io)
{
	lfs_file_t file;
	uint8_t res = ERR_OK;
	int32_t fileSize;
	int result;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)) == pdTRUE)
	{
		if (io == NULL)
		{
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED; /* printing a file without an I/O channel does not make any sense */
		}
		if (!FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: File system is not mounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		result = lfs_file_open(&FS_lfs, &file, filePath, LFS_O_RDONLY);
		if (result < 0)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: Failed opening file.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		fileSize = lfs_file_size(&FS_lfs, &file);
		if (fileSize < 0)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: getting file size\r\n", io->stdErr);
				(void) lfs_file_close(&FS_lfs, &file);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		res = CLS1_PrintMemory(&file, 0, fileSize - 1, 4, 16, readFromFile, io);
		if (res != ERR_OK)
		{
			CLS1_SendStr("ERROR while calling PrintMemory()\r\n", io->stdErr);
		}
		(void) lfs_file_close(&FS_lfs, &file);

		xSemaphoreGive(fileSystemAccessSema);
		return res;
	}
	else
	{
		return ERR_BUSY;
	}
}

uint8_t FS_RemoveFile(const char *filePath, CLS1_ConstStdIOType *io)
{
	int result;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: File system is not mounted.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}
		result = lfs_remove(&FS_lfs, filePath);
		if (result < 0)
		{
			if (io != NULL)
			{
				CLS1_SendStr("ERROR: Failed removing file.\r\n", io->stdErr);
			}
			xSemaphoreGive(fileSystemAccessSema);
			return ERR_FAILED;
		}

		xSemaphoreGive(fileSystemAccessSema);
		return ERR_OK;
	}
	else
	{
		return ERR_BUSY;
	}
}

lfs_t* FS_GetFileSystem(void)
{
	return &FS_lfs;
}

uint8_t FS_RunBenchmark(CLS1_ConstStdIOType *io)
{
	lfs_file_t file;
	int result;
	uint32_t i;
	uint8_t read_buf[10];
	TIMEREC time, startTime;
	int32_t start_mseconds, mseconds;

	if(xSemaphoreTake(fileSystemAccessSema,pdMS_TO_TICKS(500)))
	{
		if (!FS_isMounted)
			{
				if (io != NULL)
				{
					CLS1_SendStr("ERROR: File system is not mounted.\r\n", io->stdErr);
				}
				xSemaphoreGive(fileSystemAccessSema);
				return ERR_FAILED;
			}
			/* write benchmark */
			CLS1_SendStr((const unsigned char*) "Benchmark: write/copy/read a 100kB file:\r\n",	io->stdOut);
			CLS1_SendStr((const unsigned char*) "Delete existing benchmark files...\r\n",io->stdOut);
			(void) FS_RemoveFile((const unsigned char*) "./bench.txt", io);
			(void) FS_RemoveFile((const unsigned char*) "./copy.txt", io);

			CLS1_SendStr((const unsigned char*) "Create benchmark file...\r\n",	io->stdOut);
			(void) TmDt1_GetTime(&startTime);
			if (lfs_file_open(&FS_lfs, &file, "./bench.txt", LFS_O_WRONLY | LFS_O_CREAT)< 0)
			{
				CLS1_SendStr((const unsigned char*) "*** Failed creating benchmark file!\r\n",io->stdErr);
				xSemaphoreGive(fileSystemAccessSema);
				return ERR_FAILED;
			}
			for (i = 0; i < 10240; i++)
			{
				if (lfs_file_write(&FS_lfs, &file, "benchmark ",sizeof("benchmark ") - 1) < 0)
				{
					CLS1_SendStr((const unsigned char*) "*** Failed writing file!\r\n",	io->stdErr);
					(void) lfs_file_close(&FS_lfs, &file);
					xSemaphoreGive(fileSystemAccessSema);
					return ERR_FAILED;
				}
			}
			(void) lfs_file_close(&FS_lfs, &file);
			(void) TmDt1_GetTime(&time);

			start_mseconds = startTime.Hour * 60 * 60 * 1000 + startTime.Min * 60 * 1000+ startTime.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ startTime.Sec100 * 10
		#endif
			;
			mseconds = time.Hour * 60 * 60 * 1000 + time.Min * 60 * 1000+ time.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ time.Sec100 * 10
		#endif
			- start_mseconds;
			CLS1_SendNum32s(mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " ms for writing (", io->stdOut);
			CLS1_SendNum32s((100 * 1000) / mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " kB/s)\r\n", io->stdOut);

			/* read benchmark */
			CLS1_SendStr((const unsigned char*) "Read 100kB benchmark file...\r\n",	io->stdOut);
			(void) TmDt1_GetTime(&startTime);
			if (lfs_file_open(&FS_lfs, &file, "./bench.txt", LFS_O_RDONLY) < 0)
			{
				CLS1_SendStr((const unsigned char*) "*** Failed opening benchmark file!\r\n",io->stdErr);
				xSemaphoreGive(fileSystemAccessSema);
				return ERR_FAILED;
			}
			for (i = 0; i < 10240; i++)
			{
				if (lfs_file_read(&FS_lfs, &file, &read_buf[0], sizeof(read_buf)) < 0)
				{
					CLS1_SendStr((const unsigned char*) "*** Failed reading file!\r\n",	io->stdErr);
					(void) lfs_file_close(&FS_lfs, &file);
					xSemaphoreGive(fileSystemAccessSema);
					return ERR_FAILED;
				}
			}
			(void) lfs_file_close(&FS_lfs, &file);
			(void) TmDt1_GetTime(&time);
			start_mseconds = startTime.Hour * 60 * 60 * 1000 + startTime.Min * 60 * 1000+ startTime.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ startTime.Sec100 * 10
		#endif
					;
			mseconds = time.Hour * 60 * 60 * 1000 + time.Min * 60 * 1000+ time.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ time.Sec100 * 10
		#endif
					- start_mseconds;
			CLS1_SendNum32s(mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " ms for reading (", io->stdOut);
			CLS1_SendNum32s((100 * 1000) / mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " kB/s)\r\n", io->stdOut);

			/* copy benchmark */
			CLS1_SendStr((const unsigned char*) "Copy 100kB file...\r\n", io->stdOut);
			(void) TmDt1_GetTime(&startTime);
			(void) FS_CopyFile((const unsigned char*) "./bench.txt",(const unsigned char*) "./copy.txt", io);
			(void) TmDt1_GetTime(&time);
			start_mseconds = startTime.Hour * 60 * 60 * 1000 + startTime.Min * 60 * 1000+ startTime.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ startTime.Sec100 * 10
		#endif
							;
			mseconds = time.Hour * 60 * 60 * 1000 + time.Min * 60 * 1000+ time.Sec * 1000
		#if TmDt1_HAS_SEC100_IN_TIMEREC
					+ time.Sec100 * 10
		#endif
					- start_mseconds;
			CLS1_SendNum32s(mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " ms for copy (", io->stdOut);
			CLS1_SendNum32s((100 * 1000) / mseconds, io->stdOut);
			CLS1_SendStr((const unsigned char*) " kB/s)\r\n", io->stdOut);
			CLS1_SendStr((const unsigned char*) "done!\r\n", io->stdOut);

			xSemaphoreGive(fileSystemAccessSema);
			return ERR_OK;
	}

	else
	{
		return ERR_BUSY;
	}


}

static uint8_t FS_PrintStatus(CLS1_ConstStdIOType *io)
{
	uint8_t buf[24];

	CLS1_SendStatusStr((const unsigned char*) "FS",	(const unsigned char*) "\r\n", io->stdOut);
	CLS1_SendStatusStr((const unsigned char*) "  mounted",	FS_isMounted ? "yes\r\n" : "no\r\n", io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.block_count * FS_cfg.block_size);
	UTIL1_strcat(buf, sizeof(buf), " bytes\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  space", buf, io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.read_size);
	UTIL1_strcat(buf, sizeof(buf), "\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  read_size", buf, io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.prog_size);
	UTIL1_strcat(buf, sizeof(buf), "\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  prog_size", buf, io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.block_size);
	UTIL1_strcat(buf, sizeof(buf), "\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  block_size", buf, io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.block_count);
	UTIL1_strcat(buf, sizeof(buf), "\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  block_count", buf, io->stdOut);

	UTIL1_Num32uToStr(buf, sizeof(buf), FS_cfg.lookahead);
	UTIL1_strcat(buf, sizeof(buf), "\r\n");
	CLS1_SendStatusStr((const unsigned char*) "  lookahead", buf, io->stdOut);
	return ERR_OK;
}

uint8_t FS_ParseCommand(const unsigned char* cmd, bool *handled,const CLS1_StdIOType *io)
{
	unsigned char fileNameSrc[FS_FILE_NAME_SIZE],fileNameDst[FS_FILE_NAME_SIZE];
	size_t lenRead;

	if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP) == 0|| UTIL1_strcmp((char*)cmd, "FS help") == 0) {
		CLS1_SendHelpStr((unsigned char*) "FS",	(const unsigned char*) "Group of FileSystem (LittleFS) commands\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  help|status",	(const unsigned char*) "Print help or status information\r\n",	io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  format",(const unsigned char*) "Format the file system\r\n",io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  mount",(const unsigned char*) "Mount the file system\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  unmount",(const unsigned char*) "unmount the file system\r\n",	io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  ls",(const unsigned char*) "List directory and files\r\n",	io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  rm <file>",(const unsigned char*) "Remove a file\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  mv <src> <dst>",(const unsigned char*) "Rename a file\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  cp <src> <dst>",(const unsigned char*) "Copy a file\r\n", io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  printhex <file>",(const unsigned char*) "Print the file data in hexadecimal format\r\n",io->stdOut);
		CLS1_SendHelpStr((unsigned char*) "  benchmark",(const unsigned char*) "Run a benchmark to measure performance\r\n",io->stdOut);
		*handled = TRUE;
		return ERR_OK;
	}
	else if (UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS) == 0|| UTIL1_strcmp((char*)cmd, "FS status") == 0)
	{
		*handled = TRUE;
		return FS_PrintStatus(io);
	}
	else if (UTIL1_strcmp((char*)cmd, "FS format") == 0)
	{
		*handled = TRUE;
		return FS_Format(io);
	}
	else if (UTIL1_strcmp((char*)cmd, "FS mount") == 0)
	{
		*handled = TRUE;
		return FS_Mount(io);
	}
	else if (UTIL1_strcmp((char*)cmd, "FS unmount") == 0)
	{
		*handled = TRUE;
		return FS_Unmount(io);
	}
	else if (UTIL1_strcmp((char*)cmd, "FS ls") == 0)
	{
		*handled = TRUE;
		return FS_Dir(NULL, io);
	}
	else if (UTIL1_strcmp((char*)cmd, "FS benchmark") == 0)
	{
		*handled = TRUE;
		return FS_RunBenchmark(io);
	}
	else if (UTIL1_strncmp((char* )cmd, "FS printhex ",	sizeof("FS printhex ") - 1)	== 0)
	{
		*handled = TRUE;
		if ((UTIL1_ReadEscapedName(cmd + sizeof("FS printhex ") - 1,fileNameSrc, sizeof(fileNameSrc), &lenRead, NULL, NULL)	== ERR_OK))
		{
			return FS_PrintHexFile(fileNameSrc, io);
		}
		return ERR_FAILED;
	}
	else if (UTIL1_strncmp((char*)cmd, "FS rm ", sizeof("FS rm ")-1) == 0)
	{
		*handled = TRUE;
		if ((UTIL1_ReadEscapedName(cmd + sizeof("FS rm ") - 1, fileNameSrc,	sizeof(fileNameSrc), &lenRead, NULL, NULL) == ERR_OK))
		{
			return FS_RemoveFile(fileNameSrc, io);
		}
		return ERR_FAILED;
	}
	else if (UTIL1_strncmp((char*)cmd, "FS mv ", sizeof("FS mv ")-1) == 0)
	{
		*handled = TRUE;
		if ((UTIL1_ReadEscapedName(cmd + sizeof("FS mv ") - 1, fileNameSrc,	sizeof(fileNameSrc), &lenRead, NULL, NULL) == ERR_OK)
				&& *(cmd + sizeof("FS cp ") - 1 + lenRead) == ' '
				&& (UTIL1_ReadEscapedName(cmd + sizeof("FS mv ") - 1 + lenRead + 1, fileNameDst,sizeof(fileNameDst), NULL, NULL, NULL) == ERR_OK))
		{
			return FS_MoveFile(fileNameSrc, fileNameDst, io);
		}
		return ERR_FAILED;
	}
	else if (UTIL1_strncmp((char*)cmd, "FS cp ", sizeof("FS cp ")-1) == 0)
	{
		*handled = TRUE;
		if ((UTIL1_ReadEscapedName(cmd + sizeof("FS cp ") - 1, fileNameSrc,	sizeof(fileNameSrc), &lenRead, NULL, NULL) == ERR_OK)
				&& *(cmd + sizeof("FS cp ") - 1 + lenRead) == ' '
				&& (UTIL1_ReadEscapedName(cmd + sizeof("FS cp ") - 1 + lenRead + 1, fileNameDst,sizeof(fileNameDst), NULL, NULL, NULL) == ERR_OK))
		{
			return FS_CopyFile(fileNameSrc, fileNameDst, io);
		}
		return ERR_FAILED;
	}
	return ERR_OK;
}

void FS_GetFileAccessSemaphore(SemaphoreHandle_t* sema)
{
	*sema = fileSystemAccessSema;
}

uint8_t FS_Init(void)
{
	fileSystemAccessSema = xSemaphoreCreateBinary();
	xSemaphoreGive(fileSystemAccessSema);
	if (SPIF_Init() != ERR_OK)
	{
		return ERR_FAILED;
	}
	//return ERR_OK;
	return FS_Mount(NULL);
}

