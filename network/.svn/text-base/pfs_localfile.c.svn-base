/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_localfile.h"
#include "pfs_time_stamp.h"
#include "pfs_file_filter.h"
#include "pfs_timer.h"
#include "common.h"
#include "pfs_del_file.h"
#include <sys/vfs.h>
#include <utime.h>
#include <libgen.h>
extern int glogfd;
extern t_g_config g_config;

/*
 *本文件有优化余地
 */

static uint32_t g_index = 0;

static void trim_sep(char *s, char sep)
{
	char buf[1024] = {0x0};
	int l = snprintf(buf, sizeof(buf), "%s", s);
	if (l >= 1023)
		return;
	memset(s, 0, l);
	char *t = buf;
	char *last = NULL;;
	while (*t)
	{
		if (last && *last == *t && *t == sep)
		{
			t++;
			continue;
		}
		*s = *t;
		if (*t == sep)
			last = t;
		else
			last = 0x0;
		s++;
		t++;
	}
}

static int storage_convert(t_task_base *task)
{
	char realfile[256] = {0x0};
	strcat(realfile, g_config.datadir);
	char *bname = strrchr(task->filename, '/');
	if (bname == NULL)
	{
		LOG(glogfd, LOG_ERROR, "file %s err!\n", task->filename);
		return LOCALFILE_DIR_E;
	}
	char *last = bname;
	char *t = last - 1;
	while (isdigit(*t) || *t == '/')
	{
		if (*t == '/')
			last = t;
		t--;
	}
	strcat(realfile, last);
	snprintf(task->retfile, sizeof(task->retfile), "%s", last);
	memset(task->filename, 0, sizeof(task->filename));
	snprintf(task->filename, sizeof(task->filename), "%s", realfile);
	trim_sep(task->filename, '/');
	return LOCALFILE_OK;
}

static void get_tmpstr(char *d, int len)
{
	char t[16] = {0x0};
	get_strtime(t);
	snprintf(d, len, "_%s_%d", t, g_index++);
}

int createdir(char *file)
{
	char *pos = file;
	int len = 1;
	while (1)
	{
		pos = strchr(file + len, '/');
		if (!pos)
			break;
		*pos = 0;
		if (access(file, F_OK) != 0)
		{
			if (mkdir(file, 0755) < 0)
			{
				LOG(glogfd, LOG_ERROR, "mkdir [%s] [%m][%d]!\n", file, errno);
				return -1;
			}
			if (chown(file, g_config.dir_uid, g_config.dir_gid))
				LOG(glogfd, LOG_ERROR, "chown [%s] %d %d [%m][%d]!\n", file, g_config.dir_uid, g_config.dir_gid, errno);
		}

		*pos = '/';
		len = pos - file + 1;
	}
	if (chown(file, g_config.dir_uid, g_config.dir_gid))
		LOG(glogfd, LOG_ERROR, "chown [%s] %d %d [%m][%d]!\n", file, g_config.dir_uid, g_config.dir_gid, errno);
	return 0;
}

void real_rm_file(char *file)
{
	int ret = unlink(file);
	if (ret && errno != ENOENT)
	{
		LOG(glogfd, LOG_ERROR, "file [%s] unlink err %m\n", file);
		return ;
	}
	LOG(glogfd, LOG_NORMAL, "file [%s] be unlink\n", file);
}

int delete_localfile(t_task_base *task)
{
	if (storage_convert(task) != LOCALFILE_OK)
		return LOCALFILE_DIR_E;
	char rmfile[256] = {0x0};
	snprintf(rmfile, sizeof(rmfile), "%s.tmp4rm", task->filename);
	if (rename(task->filename, rmfile))
	{
		LOG(glogfd, LOG_ERROR, "file [%s] rename [%s]err %m\n", task->filename, rmfile);
		return LOCALFILE_RENAME_E;
	}
	add_2_del_file (task);
	t_pfs_timer pfs_timer;
	memset(&pfs_timer, 0, sizeof(pfs_timer));
	snprintf(pfs_timer.args, sizeof(pfs_timer.args), "%s", rmfile);
	pfs_timer.span_time = g_config.real_rm_time;
	pfs_timer.cb = real_rm_file;
	if (add_to_delay_task(&pfs_timer))
		LOG(glogfd, LOG_ERROR, "file [%s]add delay task err %m\n", rmfile);
	return LOCALFILE_OK;
}

int check_localfile_md5(t_task_base *task, int type)
{
	char *outdir;
	outdir = task->filename;
	if (type == VIDEOTMP)  /*tmpfile = "." + file + ".pfs"; */
		outdir = task->tmpfile;
	else
		storage_convert(task);
	unsigned char md5[17] = {0x0};
	if (getfilemd5(outdir, md5))
		return LOCALFILE_FILE_E;
	if (memcmp(md5, task->filemd5, sizeof(task->filemd5)))
	{
		LOG(glogfd, LOG_ERROR, "file [%s], md5 err\n", outdir);
		return LOCALFILE_MD5_E;
	}
	struct utimbuf c_time;
	c_time.actime = task->mtime;
	c_time.modtime = c_time.actime;
	utime(outdir, &c_time);
	return LOCALFILE_OK;
}

int open_localfile_4_read(t_task_base *task, int *fd)
{
	*fd = open(task->filename, O_RDONLY);
	if (*fd < 0)
	{
		LOG(glogfd, LOG_ERROR, "open %s err %m\n", task->filename);
		return LOCALFILE_OPEN_E;
	}
	return LOCALFILE_OK;
}

int open_tmp_localfile_4_write(t_task_base *task, int *fd)
{
	if (storage_convert(task) != LOCALFILE_OK)
		return LOCALFILE_DIR_E;
	memset(task->tmpfile, 0, sizeof(task->tmpfile));
	snprintf(task->tmpfile, sizeof(task->tmpfile), "%s", task->filename);
	char *outdir = task->tmpfile;
	strcat(outdir, ".");
	char tmpstr[32] = {0x0};
	get_tmpstr(tmpstr, sizeof(tmpstr));
	strcat(outdir, tmpstr);
	strcat(outdir, ".pfs");
	*fd = open(outdir, O_CREAT | O_RDWR | O_LARGEFILE, 0644);
	if (*fd < 0 && errno == ENOENT)
	{
		char *t = strrchr(task->filename, '/');
		if (t == NULL)
		{
			LOG(glogfd, LOG_ERROR, "file %s err!\n", task->filename);
			return LOCALFILE_DIR_E;
		}
		*t = 0x0;
		if (access(task->filename, F_OK) != 0)
		{
			LOG(glogfd, LOG_DEBUG, "dir %s not exist, try create !\n", task->filename);
			if (createdir(task->filename))
			{
				*t = '/';
				LOG(glogfd, LOG_ERROR, "dir %s create %m!\n", task->filename);
				return LOCALFILE_DIR_E;
			}
		}
		*t = '/';
		*fd = open(outdir, O_CREAT | O_RDWR | O_LARGEFILE, 0644);
		if (*fd < 0)
		{
			LOG(glogfd, LOG_ERROR, "open %s err %m\n", outdir);
			return LOCALFILE_OPEN_E;
		}
	}
	return LOCALFILE_OK;
}

int close_tmp_check_mv(t_task_base *task, int fd, unsigned char *md5)
{
	if (fd < 0)
	{
		LOG(glogfd, LOG_ERROR, "fd < 0 %d %s\n", fd, task->filename);
		return LOCALFILE_OPEN_E;
	}
	close(fd);
	if (memcmp(md5, task->filemd5, sizeof(task->filemd5)) == 0)
	{
		struct utimbuf c_time;
		c_time.actime = task->mtime;
		c_time.modtime = c_time.actime;
		utime(task->tmpfile, &c_time);
	}
	else
	{
		int ret = check_localfile_md5(task, VIDEOTMP);
		if (ret != LOCALFILE_OK)
		{
			LOG(glogfd, LOG_ERROR, "check_localfile_md5 ERR %s\n", task->filename);
			return ret;
		}
	}

	if (rename(task->tmpfile, task->filename))
	{
		LOG(glogfd, LOG_ERROR, "rename %s to %s err %m\n", task->tmpfile, task->filename);
		return LOCALFILE_RENAME_E;
	}
	set_time_stamp(task);
	if (chown(task->filename, g_config.dir_uid, g_config.dir_gid))
		LOG(glogfd, LOG_ERROR, "chown [%s] %d %d [%m][%d]!\n", task->filename, g_config.dir_uid, g_config.dir_gid, errno);
	return LOCALFILE_OK;
}

int check_disk_space(t_task_base *base)
{
	char path[256] = {0x0};
	char *t = strrchr(base->filename, '/');
	if (t == NULL)
	{
		LOG(glogfd, LOG_ERROR, "err %s \n", base->filename);
		return DISK_ERR;
	}
	strncpy(path, base->filename, t - base->filename);
	if (access(path, F_OK) != 0)
	{
		LOG(glogfd, LOG_DEBUG, "dir %s not exist, try create !\n", path);
		if (createdir(path))
		{
			LOG(glogfd, LOG_ERROR, "dir %s create %m!\n", path);
			return LOCALFILE_DIR_E;
		}
	}
	struct statfs sfs;
	if (statfs(path, &sfs))
	{
		LOG(glogfd, LOG_ERROR, "statfs %s err %m\n", path);
		return DISK_ERR;
	}

	uint64_t diskfree = sfs.f_bfree * sfs.f_bsize;
	if (g_config.mindiskfree > diskfree)
	{
		LOG(glogfd, LOG_ERROR, "statfs %s space not enough %llu:%llu\n", path, diskfree, g_config.mindiskfree);
		return DISK_SPACE_TOO_SMALL;
	}
	return DISK_OK;
}

int get_localfile_stat(t_task_base *task)
{
	struct stat filestat;
	if (stat(task->filename, &filestat))
	{
		LOG(glogfd, LOG_ERROR, "stat file %s err %m!\n", task->filename);
		return LOCALFILE_DIR_E;
	}
	task->mtime = filestat.st_mtime;
	task->fsize = filestat.st_size;
	if (getfilemd5(task->filename, task->filemd5))
		return LOCALFILE_FILE_E;
	return LOCALFILE_OK;
}

static time_t get_dir_lasttime(char *path)
{
	time_t now = time(NULL); 
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(path)) == NULL) 
	{
		LOG(glogfd, LOG_ERROR, "opendir %s err  %m\n", path);
		return now;
	}
	LOG(glogfd, LOG_TRACE, "opendir %s ok \n", path);
	time_t maxtime = 0;
	while((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_name[0] == '.')
			continue;
		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s/%s", path, dirp->d_name);
		if (check_file_filter(dirp->d_name))
			continue;

		struct stat filestat;
		if(stat(file, &filestat) < 0) 
		{
			LOG(glogfd, LOG_ERROR, "stat error,filename:%s\n", file);
			continue;
		}
		if (filestat.st_ctime >= maxtime)
			maxtime = filestat.st_ctime;
	}
	closedir(dp);
	return maxtime;
}

time_t get_storage_dir_lasttime(int d1, int d2)
{
	char outdir[256] = {0x0};
	char *datadir = myconfig_get_value("pfs_datadir");
	sprintf(outdir, "%s/%d/%d/", datadir, d1, d2);
	return get_dir_lasttime(outdir);
}


