#include "vsfs.h"
#include "vsfs-errors.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

const char *start_marker = "MYFSPT\0";

static int cwd;
static int dev_id;
static struct header h;
static int descrs_tab[MAX_FILES_OPENED];

int vs_mkfs(char *filename, int dev_size){
    dev_id = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    if (dev_id < 0) return -CREATE_ERR;

    if (write(dev_id, start_marker, sizeof(start_marker)) < 0)
        return -WRITE_ERR;

    /*
      Total number or blocks for files in an image for chosen dev_size
      Considering that image consists of the:
      marker, header, free blocks bitmap, inode table and file blocks
      max number of files is taken as equal to nblocks/2
    */
    int nblocks = (dev_size - sizeof(start_marker) - sizeof(struct header))
        / (BLOCK_SIZE + sizeof(char) + sizeof(struct fstat)/2);

    if (nblocks < 2) return -SIZE_ERR;

    h.dev_size = dev_size;
    h.block_size = BLOCK_SIZE;
    h.nblocks = nblocks;
    h.nfiles_max = nblocks / 2;
    if (write(dev_id, &h, sizeof(h)) < 0)
        return -WRITE_ERR;

    char *bitmap_buf = malloc(nblocks * sizeof(char));
    memset(bitmap_buf, 0, nblocks);
    bitmap_buf[0] = 1;
    if (write(dev_id, bitmap_buf, nblocks) < 0) {
        free(bitmap_buf);
        return -WRITE_ERR;
    }
    free(bitmap_buf);

    int fstattab_size = h.nfiles_max * sizeof(struct fstat);
    struct fstat *fstat_buf = malloc(fstattab_size);

    fstat_buf[0].ftype = DIRTYPE;
    fstat_buf[0].nlinks = 2;
    fstat_buf[0].size = 0;
    fstat_buf[0].blocks_map[0] = 0;
    for (int j = 1; j < FILE_BLOCKS; j++)
        fstat_buf[0].blocks_map[j] = -1;
    struct dir_rec self = {
        .id = 0
    };
    self.name[0] = '.';
    for (int i = 1; i < MAX_NAMESIZE; i++)
        self.name[i] = '\0';
    
    struct dir_rec parent = {
        .id = 0
    };
    parent.name[0] = '.';
    parent.name[1] = '.';
    for (int i = 2; i < MAX_NAMESIZE; i++)
        parent.name[i] = '\0';

    struct dir_rec end_rec = {
        .id = END_ID
    };
    for (int i = 0; i < MAX_NAMESIZE; i++)
        end_rec.name[i] = '\0';

    for (int i = 1; i < h.nfiles_max; i++) {
            fstat_buf[i].ftype = -1;
            fstat_buf[i].nlinks = 0;
            fstat_buf[i].size = 0;
            for (int j = 0; j < FILE_BLOCKS; j++)
                fstat_buf[i].blocks_map[j] = -1;
    }
    if (write(dev_id, fstat_buf, fstattab_size) < 0) {
        free(fstat_buf);
        return -WRITE_ERR;
    }
    free(fstat_buf);

    int nblocks_size = h.block_size * h.nblocks;
    char *blocks_buf = calloc(nblocks_size, 1);
    if (write(dev_id, blocks_buf, nblocks_size) < 0) {
        free(blocks_buf);
        return -WRITE_ERR;
    }
    free(blocks_buf);

    if (lseek(dev_id, get_blocks_offset(), SEEK_SET) < 0 || 
            write(dev_id, &self, sizeof(struct dir_rec)) < 0) {
        return -WRITE_ERR;
    }
    if (write(dev_id, &parent, sizeof(struct dir_rec)) < 0) {
        return -WRITE_ERR;
    }
    if (write(dev_id, &end_rec, sizeof(struct dir_rec)) < 0) {
        return -WRITE_ERR;
    }
    return 0;
}

int vs_mount(char *filename) {
    dev_id = open(filename, O_RDWR, S_IRWXU);
    if (dev_id < 0) return -OPEN_ERR;

    int marker_size = sizeof(start_marker);
    char *marker = malloc(marker_size);

    if (read(dev_id, marker, marker_size) < 0) {
        free(marker);
        return -READ_ERR;
    }

    if (0 != strcmp(marker, start_marker)) {
        free(marker);
        return -MARKER_ERR;
    }
    free(marker);

    if (read(dev_id, &h, sizeof(struct header)) < 0)
        return -READ_ERR;

    for (int i = 0; i < MAX_FILES_OPENED; i++)
        descrs_tab[i] = -1;
    cwd = 0;

    return 0;
}

int vs_umount() {
    h.dev_size = -1;
    h.block_size = -1;
    h.nblocks = -1;
    h.nfiles_max = -1;
    for (int i = 0; i < MAX_FILES_OPENED; i++)
        if (descrs_tab[i] >= 0) vs_close(i);

    if (close(dev_id) < 0) return -CLOSE_ERR;
    dev_id = -1;
    cwd = -1;

    return 0;
}

int vs_getstat(int id, struct fstat *stat) {
    int fstat_offset =
                get_fstattab_offset() + id * sizeof(struct fstat);

    if (lseek(dev_id, fstat_offset, SEEK_SET) < 0)
        return -READ_ERR;

    if (read(dev_id, stat, sizeof(struct fstat)) < 0)
        return -READ_ERR;

    return 0;
}

//if next == 0 then first record is read, 
//else all the records are read sequentially
int readdir_state, readdir_i;
int vs_readdir(char *pathname, struct dir_rec *dir_rec, int next) {
    int dir_id = resolve_path(pathname, cwd);
    if (dir_id < 0)
        return -NOTEXIST_ERR;

    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(dir_id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }

    if (stat->ftype != DIRTYPE) {
        free(stat);
        return -NOTDIR_ERR;
    }

    if (!next) {
        readdir_state = 0;
        readdir_i = 0;
        int blockid = stat->blocks_map[0];
        int read_offset = get_blocks_offset()
                            + blockid * h.block_size;
        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
    }

    if (read(dev_id, dir_rec, sizeof(struct dir_rec)) < 0) {
        free(stat);
        return -READ_ERR;
    }

    if (readdir_state >= h.block_size/sizeof(struct dir_rec)) {
        readdir_state = 0;
        int blockid = get_block_id(stat, ++readdir_i, 0);
        if (blockid < 0) {
            free(stat);
            return -EOF_ERR;
        }
        int read_offset = get_blocks_offset()
                            + blockid * h.block_size;
        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
    }
    
    free(stat);
    return 0;
}

int vs_create(char *pathname) {
    int dir_id = resolve_path(dirname(pathname), cwd);
    if (dir_id < 0) return -NOTEXIST_ERR;
    char *name = malloc(MAX_NAMESIZE);
    int f_id = resolve_path(pathname, cwd);
    if (f_id >= 0) {
        free(name);
        return -EXIST_ERR;
    }
    strcpy(name, basename(pathname));

    int fstattab_size = sizeof(struct fstat) * h.nfiles_max;
    struct fstat *fstattab = malloc(fstattab_size);
    if (read_fstattab(fstattab) < 0) {
        free(fstattab);
        return -READ_ERR;
    }

    int id;
    for (id = 0; id < h.nfiles_max && fstattab[id].nlinks > 0; id++)
        ;
    free(fstattab);
    if (id >= h.nfiles_max) return -MAXFILES_ERR;

    struct fstat stat = {
        .ftype = REGTYPE,
        .nlinks = 1,
        .size = 0
    };
    for (int j = 0; j < FILE_BLOCKS; j++)
        stat.blocks_map[j] = -1;

    struct dir_rec dirrec = {
        .id = id
    };

    int j;
    for (j = 0; j < MAX_NAMESIZE && name[j] > 0; j++)
        dirrec.name[j] = name[j];

    for (; j < MAX_NAMESIZE; j++)
        dirrec.name[j] = 0;

    if (write_fstat(&stat, id) < 0)
        return -WRITE_ERR;

    if (write_dir_rec(dir_id, &dirrec) < 0)
        return -WRITE_ERR;

    return 0;
}

int vs_open(char *pathname) {
    int dir_id = resolve_path(dirname(pathname), cwd);
    if (dir_id < 0) return -NOTEXIST_ERR;
    char *name = malloc(MAX_NAMESIZE);
    strcpy(name, basename(pathname));

    int id = resolve_path(pathname, cwd);
    if (id < 0) {
        free(name);
        return -NOTEXIST_ERR;
    }
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    if (stat->ftype != REGTYPE || stat->ftype != SYMTYPE) {
        free(stat);
        free(name);
        return -NOTREG_ERR;
    }
    free(stat);
    int desc = next_descriptor();
    if (desc < 0) {
        free(name);
        return -MAX_FOPENED_ERR;
    }
    descrs_tab[desc] = id;
    free(name);
    return desc;
}

int vs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES_OPENED || descrs_tab[fd] == -1)
        return -BADDESC_ERR;

    descrs_tab[fd] = -1;
    return 0;
}

int vs_read(int fd, int offset, int size, char *buffer) {
    if (fd < 0 || fd >= MAX_FILES_OPENED || descrs_tab[fd] == -1)
        return -BADDESC_ERR;

    int id = descrs_tab[fd];
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    
    if (offset >= stat->size) {
        free(stat);
        return 0;
    }

    int block_offset = offset / h.block_size;
    int byte_offset = offset - block_offset * h.block_size;

    int full_size = size;

    while (size > 0) {
        int blockid = get_block_id(stat, block_offset, 0);

        if (blockid < 0) {
            free(stat);
            return full_size - size;
        }
        int read_offset = get_blocks_offset()
                          + blockid * h.block_size
                          + byte_offset;

        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
        
        int rem = h.block_size - byte_offset;
        if (rem >= size) {
            int rsize;
            if ((rsize = read(dev_id, buffer, size)) < 0){
                free(stat);
                return -READ_ERR;
            }
            size -= rsize;
            free(stat);
            return full_size - size;
        } else {
            int rsize;
            if ((rsize = read(dev_id, buffer, rem)) < 0) {
                free(stat);
                return -READ_ERR;
            }
            if (rsize == 0) {
                free(stat);
                return full_size - size;
            }
            buffer += rsize;
            size -= rsize;
            byte_offset = 0;
            block_offset++;
        }
    }
    free(stat);
    return full_size;
}

int vs_write(int fd, int offset, int size, char *buffer) {
    if (fd < 0 || fd >= MAX_FILES_OPENED || descrs_tab[fd] == -1)
        return -BADDESC_ERR;

    int id = descrs_tab[fd];
    struct fstat *stat = malloc(sizeof(struct fstat));

    int fstat_offset = get_fstattab_offset()
                       + id * sizeof(struct fstat);

    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }

    int writesize = size;
    int null_size = 0;
    if (offset > stat->size) {
        null_size = offset - stat->size;
        writesize += null_size;
        offset = stat->size;
    }
    char *writebuf = malloc(writesize * sizeof(char));
    int i;
    for (i = 0; i < null_size; i++)
        writebuf[i] = '\0';
    for (; i < writesize; i++)
        writebuf[i] = buffer[i];

    int block_offset = offset / h.block_size;
    int byte_offset = offset - block_offset * h.block_size;

    int full_size = writesize;

    while (writesize > 0) {
        int blockid = get_block_id(stat, block_offset, 1);


        if (blockid < 0) {
            if (blockid == -EOF_ERR 
                || blockid == -1) return full_size - writesize;
            else return blockid;
        }
        
        int write_offset = get_blocks_offset()
                           + blockid * h.block_size
                           + byte_offset;

        if (lseek(dev_id, write_offset, SEEK_SET) < 0) {
            free(stat);
            free(writebuf);
            return -WRITE_ERR;
        }

        int rem = h.block_size - byte_offset;
        if (rem >= writesize) {
            int wsize;
            if ((wsize = write(dev_id, writebuf, writesize)) < 0){
                free(stat);
                free(writebuf);
                return -WRITE_ERR;
            }
            if (stat->size < offset + wsize) {
                stat->size = offset + wsize;
                if (write_fstat(stat, id)) {
                    free(stat);
                    free(writebuf);
                    return -WRITE_ERR;
                }
            }
            writesize -= wsize;
            free(stat);
            free(writebuf);
            return full_size - writesize;
        } else {
            int wsize;
            if ((wsize = write(dev_id, writebuf, rem)) < 0) {
                free(stat);
                free(writebuf);
                return -WRITE_ERR;
            }
            if (wsize == 0) {
                free(stat);
                free(writebuf);
                return full_size - writesize;
            }
            writebuf += wsize;
            writesize -= wsize;
            byte_offset = 0;
            block_offset++;
            offset += h.block_size;
            if (stat->size < offset + wsize) {
                stat->size = offset + wsize;
                if (write_fstat(stat, id)) {
                    free(stat);
                    free(writebuf);
                    return -WRITE_ERR;
                }
            }
        }
    }
    free(stat);
    return full_size;
}

int vs_link(char *src_pathname, char *dest_pathname) {
    int src_id = resolve_path(src_pathname, cwd);
    int dest_dir_id = resolve_path(dirname(dest_pathname), cwd);
    if (src_id < 0 || dest_dir_id < 0)
        return -NOTEXIST_ERR;
    
    int dest_id = get_rec_id(dest_dir_id, basename(dest_pathname));
    if (dest_id > 0)
        return -EXIST_ERR;

    struct dir_rec dirrec = {
        .id = src_id
    };
    int j;
    for (j = 0; j < MAX_NAMESIZE && basename(dest_pathname)[j] > 0; j++)
        dirrec.name[j] = basename(dest_pathname)[j];

    for (; j < MAX_NAMESIZE; j++)
        dirrec.name[j] = 0;

    if (write_dir_rec(dest_dir_id, &dirrec) < 0)
        return -WRITE_ERR;
    return 0;
}

int vs_unlink(char *pathname) {
    int dir_id = resolve_path(dirname(pathname), cwd);
    int id = get_rec_id(dir_id, basename(pathname));
    if (id < 0 || dir_id < 0)
        return -NOTEXIST_ERR;

    if (rm_rec(dir_id, basename(pathname)) < 0)
        return -WRITE_ERR;
    
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    stat->nlinks--;
    if (stat->nlinks == 0) {
        stat->ftype = -1;
        stat->size = 0;
        for (int j= 0; j < FILE_BLOCKS-1; j++) {
            free_block(stat->blocks_map[j]);
            stat->blocks_map[j] = -1;
        }

        if (stat->blocks_map[FILE_BLOCKS-1] >= 0) {
            int *blocks = malloc(h.block_size);
            if (free_all_under_from(stat->blocks_map[FILE_BLOCKS-1], 0) < 0) {
                free(blocks);
                free(stat);
                return -WRITE_ERR;
            }
            free(blocks);
            stat->blocks_map[FILE_BLOCKS-1] = -1;
        }
    }
    if (write_fstat(stat, id) < 0) {
        free(stat);
        return -WRITE_ERR;
    }
    free(stat);
    return 0;
}

int vs_truncate(char *pathname, int size) {
    int id = resolve_path(pathname, cwd);
    if (id < 0)
        return -NOTEXIST_ERR;

    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    if (stat->ftype != REGTYPE) {
        free(stat);
        return -NOTREG_ERR;
    }
    if (size < stat->size) {
        int new_block_size = size / h.block_size;
        int old_block_size = stat->size / h.block_size;

        if (old_block_size < FILE_BLOCKS-1) {
            for (int i = new_block_size + 1; i < FILE_BLOCKS-1; i++) {
                if (stat->blocks_map[i] == -1) break;
                free_block(stat->blocks_map[i]);
                stat->blocks_map[i] = -1;
            }
        } else {
            if (new_block_size < FILE_BLOCKS-1) {
                for (int i = new_block_size + 1; i < FILE_BLOCKS-1; i++) {
                    free_block(stat->blocks_map[i]);
                    stat->blocks_map[i] = -1;
                }
                free_all_under_from(stat->blocks_map[FILE_BLOCKS-1], 0);
                stat->blocks_map[FILE_BLOCKS-1] = -1;
            } else {
                free_all_under_from(stat->blocks_map[FILE_BLOCKS-1], new_block_size-FILE_BLOCKS+2);
            }
        }
        stat->size = size;
        write_fstat(stat, id);
        free(stat);
        return 0;
    } else if (size > stat->size) {
        int fd = vs_open(pathname);
        if (fd < 0 || vs_write(fd, size, 0, NULL) < 0) {
            free(stat);
            vs_close(fd);
            return -WRITE_ERR;
        }
        vs_close(fd);
        free(stat);
        return 0;
    } else {
        free(stat);
        return 0;
    }
}

int vs_mkdir(char *pathname) {
    int dir_id = resolve_path(dirname(pathname), cwd);
    if (dir_id < 0) return -NOTEXIST_ERR;
    char *name = malloc(MAX_NAMESIZE);
    int f_id = resolve_path(pathname, cwd);
    if (f_id >= 0) {
        free(name);
        return -EXIST_ERR;
    }
    strcpy(name, basename(pathname));

    int fstattab_size = sizeof(struct fstat) * h.nfiles_max;
    struct fstat *fstattab = malloc(fstattab_size);
    if (read_fstattab(fstattab) < 0) {
        free(fstattab);
        return -READ_ERR;
    }
    int id;
    for (id = 0; id < h.nfiles_max && fstattab[id].nlinks > 0; id++)
        ;
    free(fstattab);
    if (id >= h.nfiles_max) return -MAXFILES_ERR;
    
    struct fstat stat = {
        .ftype = DIRTYPE,
        .nlinks = 2,
        .size = 0
    };
    stat.blocks_map[0] = 0;
    for (int j = 1; j < FILE_BLOCKS; j++)
        stat.blocks_map[j] = -1;
    struct dir_rec self = {
        .id = id
    };
    self.name[0] = '.';
    for (int i = 1; i < MAX_NAMESIZE; i++)
        self.name[i] = '\0';
    
    struct dir_rec parent = {
        .id = dir_id
    };
    parent.name[0] = '.';
    parent.name[1] = '.';
    for (int i = 2; i < MAX_NAMESIZE; i++)
        parent.name[i] = '\0';

    struct dir_rec end_rec = {
        .id = END_ID
    };

    for (int i = 0; i < MAX_NAMESIZE; i++)
        end_rec.name[i] = '\0';


    struct dir_rec dirrec = {
        .id = id
    };
    int j;
    for (j = 0; j < MAX_NAMESIZE && name[j] > 0; j++)
        dirrec.name[j] = name[j];

    for (; j < MAX_NAMESIZE; j++)
        dirrec.name[j] = 0;

    if (write_fstat(&stat, id) < 0)
        return -WRITE_ERR;

    if (write_dir_rec(dir_id, &dirrec) < 0)
        return -WRITE_ERR;

    if (write_dir_rec(dir_id, &self) < 0)
        return -WRITE_ERR;
    
    if (write_dir_rec(id, &parent) < 0)
        return -WRITE_ERR;
    
    if (write_dir_rec(id, &end_rec) < 0)
        return -WRITE_ERR;

    return 0;
}

int vs_rmdir(char *pathname) {
    int id = resolve_path(pathname, cwd);
    int dir_id = resolve_path(dirname(pathname), cwd);
    if (id < 0 || dir_id < 0)
        return -NOTEXIST_ERR;

    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }

    if (stat->ftype != DIRTYPE) {
        free(stat);
        return -NOTDIR_ERR;
    }

    if (stat->size > 0) {
        free(stat);
        return -DIRNOTFREE_ERR;
    }

    if (rm_rec(dir_id, basename(pathname)) < 0)
        return -WRITE_ERR;

    stat->nlinks = 0;
    stat->ftype = -1;
    stat->size = 0;
    for (int j= 0; j < FILE_BLOCKS-1; j++) {
        free_block(stat->blocks_map[j]);
        stat->blocks_map[j] = -1;
    }

    if (stat->blocks_map[FILE_BLOCKS-1] >= 0) {
        int *blocks = malloc(h.block_size);
        if (free_all_under_from(stat->blocks_map[FILE_BLOCKS-1], 0) < 0) {
            free(blocks);
            free(stat);
            return -WRITE_ERR;
        }
        free(blocks);
        stat->blocks_map[FILE_BLOCKS-1] = -1;
    }
    
    if (write_fstat(stat, id) < 0) {
        free(stat);
        return -WRITE_ERR;
    }
    free(stat);
    return 0;
}

int vs_cd(char *pathname) {
    int id = resolve_path(pathname, cwd);
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    if (stat->ftype != DIRTYPE) {
        free(stat);
        return -NOTDIR_ERR;
    } else {
        free(stat);
        cwd = id;
        return 0;
    }
}

int vs_symlink(char *src_pathname, char *dest_pathname) {
    if (vs_create(dest_pathname) < 0) {
        return -CREATE_ERR;
    }
    int fd = vs_open(dest_pathname);
    if (vs_write(fd, 0, strlen(src_pathname), src_pathname) < 0) {
        vs_close(fd);
        return -WRITE_ERR;
    }
    vs_close(fd);
}


int next_descriptor() {
    int i = 0;
    for (i = 0; i < MAX_FILES_OPENED && descrs_tab[i] != -1; i++)
        ;
    if (i >= MAX_FILES_OPENED) return -1;
    else return i;
}

int next_free_block() {
    int read_offset = sizeof(start_marker) + sizeof(struct header);

    if (lseek(dev_id, read_offset, SEEK_SET) < 0)
        return -READ_ERR;
    

    char *blocks_bitmap = malloc(h.nblocks * sizeof(char));
    if (read(dev_id, blocks_bitmap, h.nblocks * sizeof(char)) < 0) {
        free(blocks_bitmap);
        return -READ_ERR;
    }
    int i;
    for (i = 0; i < h.nblocks && blocks_bitmap[i] != 0; i++)
        ;
    free(blocks_bitmap);
    if (i >= h.nblocks) return -1;
    else return i;
}

int occupy_block(int i) {
    int write_offset = sizeof(start_marker)
                       + sizeof(struct header)
                       + i * sizeof(char);
    char c;
    if (lseek(dev_id, write_offset, SEEK_SET) < 0)
        return -READ_ERR;
    if (read(dev_id, &c, 1) < 0)
        return -READ_ERR;

    if (c) return -WRITE_ERR;

    char one = 1;

    if (lseek(dev_id, write_offset, SEEK_SET) < 0)
        return -WRITE_ERR;
    if (write(dev_id, &one, 1) < 0)
        return -WRITE_ERR;
    
    return 0;
}

int occupy_next_block() {
    int new_blockid = next_free_block();
    if (new_blockid < 0 || occupy_block(new_blockid) < 0)
        return -EOF_ERR;
                
    return new_blockid;
}

int get_fstattab_offset() {
    return sizeof(start_marker) 
            + sizeof(struct header) 
            + h.nblocks * sizeof(char);
}

int get_dirtab_offset() {
    return sizeof(start_marker)
            + sizeof(struct header)
            + h.nblocks * sizeof(char)
            + h.nfiles_max * sizeof(struct fstat);
}

int get_blocks_offset() {
    return sizeof(start_marker)
            + sizeof(struct header)
            + h.nblocks * sizeof(char)
            + h.nfiles_max * sizeof(struct fstat)
            + (h.nfiles_max + 1) * sizeof(struct dir_rec);
}

int read_fstattab(struct fstat *fstattab) {
    int fstattab_size = sizeof(struct fstat) * h.nfiles_max;

    if (lseek(dev_id, get_fstattab_offset(), SEEK_SET) < 0)
        return -READ_ERR;

    if (read(dev_id, fstattab, fstattab_size) < 0)
        return -READ_ERR;

    return 0;
}

int write_fstat(struct fstat *stat, int id) {
    if (lseek(dev_id, get_fstattab_offset() + id * sizeof(struct fstat), SEEK_SET) < 0
            || write(dev_id, stat, sizeof(struct fstat)) < 0)
        return -WRITE_ERR;
    
    return 0;
}

int write_dir_rec(int dir_id, struct dir_rec *dirrec) {
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(dir_id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    int block_offset = 0;
    int cnt = 0;
    int blockid = get_block_id(stat, block_offset, 1);

    while (1) {
        int read_offset = get_blocks_offset()
                          + blockid * h.block_size;

        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
        
        struct dir_rec *buffer = malloc(sizeof(struct dir_rec));
        
        int read_size = h.block_size;
        while (read_size > 0) {
            int rsize;
            if ((rsize=read(dev_id, buffer, sizeof(struct dir_rec))) < 0) {
                free(stat);
                free(buffer);
                return -READ_ERR;
            }
            if (buffer->id < 0) {
                if (lseek(dev_id, -sizeof(struct dir_rec), SEEK_CUR) < 0) {
                    free(stat);
                    free(buffer);
                    return -READ_ERR;
                }
                if (write(dev_id, dirrec, sizeof(struct dir_rec)) < 0) {
                    free(stat);
                    free(buffer);
                    return -WRITE_ERR;
                }
                free(stat);
                free(buffer);
                return 0;
            }
            read_size -= rsize;
        }
        free(buffer);
        block_offset++;
        blockid = get_block_id(stat, block_offset, 1);
        if (blockid < 0) {
            free(stat);
            return -1;
        }
    }
}

int get_block_id(struct fstat *stat, int block_offset, int create) {
    if (block_offset < FILE_BLOCKS-1) {
        int id = stat->blocks_map[block_offset];
        if (id >= 0 || !create) {
            return id;
        } else {
            int new_blockid = occupy_next_block();
            if (new_blockid < 0) return -EOF_ERR;
            stat->blocks_map[block_offset] = new_blockid;
            return new_blockid;
        }
    } else {
        if (block_offset-FILE_BLOCKS+1 >= h.block_size / sizeof(int))
            return -EOF_ERR;
            
        if (stat->blocks_map[FILE_BLOCKS-1] < 0) {
            if (!create) {
                return -EOF_ERR;
            } else {
                int new_blockid = occupy_next_block();
                if (new_blockid < 0) return -EOF_ERR;
                stat->blocks_map[FILE_BLOCKS-1] = new_blockid;
                int *blocks = malloc(h.block_size);
                blocks[0] = occupy_next_block();
                for (int i = 1; i < h.block_size/sizeof(int); i++)
                    blocks[i] = -1;
                
                if (lseek(dev_id, get_blocks_offset() + new_blockid*h.block_size, SEEK_SET) < 0
                        || write(dev_id, blocks, h.block_size) < 0) {
                    free(blocks);
                    return -WRITE_ERR;
                }
                free(blocks);
            }
        }

        int *blocks = malloc(h.block_size);
        int read_offset = 
                get_blocks_offset() + stat->blocks_map[FILE_BLOCKS-1] * h.block_size;
        if (lseek(dev_id, read_offset, SEEK_SET) < 0 
                || read(dev_id, blocks, h.block_size) < 0) {
            free(blocks);
            return -READ_ERR;
        }
        int id = blocks[block_offset-FILE_BLOCKS+1];
        if (id >= 0 || !create) {
            free(blocks);
            return id;
        } else {
            int new_blockid = occupy_next_block();
            blocks[block_offset-FILE_BLOCKS+1] = new_blockid;
            if (lseek(dev_id, get_blocks_offset()+stat->blocks_map[FILE_BLOCKS-1]*h.block_size, SEEK_SET) < 0
                    || write(dev_id, blocks, h.block_size) < 0) {
                free(blocks);
                return -WRITE_ERR;
            }
            free(blocks);
            return new_blockid;
        }
    }
}

int free_block(int blockid) {
    if (blockid < 0) {
        return 0;
    }
    int write_offset = sizeof(start_marker)
                        + sizeof(struct header)
                        + blockid * sizeof(char);
    char c = 0;
    if (lseek(dev_id, write_offset, SEEK_SET) < 0 || write(dev_id, &c, 1) < 0)
        return -WRITE_ERR;
    return 0;
}

int free_all_under_from(int blockid, int start) {
    int *blocks = malloc(h.block_size);
    if (lseek(dev_id, get_blocks_offset() + blockid*h.block_size, SEEK_SET) < 0
                || read(dev_id, blocks, h.block_size) < 0) {
        free(blocks);
        return -WRITE_ERR;
    }
    for (int i = start; i < h.block_size/sizeof(int); i++) {
        int block_id = blocks[i];
        if (block_id < 0)
            break;
        
        blocks[i] = -1;
        if (free_block(block_id) < 0) {
            free(blocks);
            return -WRITE_ERR;
        }
    }
    if (lseek(dev_id, get_blocks_offset() + blockid*h.block_size, SEEK_SET) < 0
                || write(dev_id, blocks, h.block_size) < 0) {
        free(blocks);
        return -WRITE_ERR;
    }
    free(blocks);
    return 0;
}

int resolve_path(char *pathname, int cwd) {
    if (strlen(pathname) == 0) return cwd;
    

    if (pathname[0] == '/') {
        return resolve_path(pathname + 1, 0);
    } else {
        char *token = strtok(pathname, "/");
        int tok_id = get_rec_id(cwd, token);
        pathname += strlen(token);
        struct fstat *stat = malloc(sizeof(struct fstat));
        if (tok_id >= 0) {
            if (vs_getstat(tok_id, stat) < 0)
                return -1;
            if (strlen(pathname) > 0 && stat->ftype == REGTYPE) {
                free(stat);
                return -1;
            } else if (strlen(pathname) > 0 && stat->ftype == DIRTYPE) {
                free(stat);
                return resolve_path(pathname, tok_id);
            } else if (stat->ftype == SYMTYPE) {
                char *name_buf = malloc(h.block_size + strlen(pathname));
                get_sym_data(tok_id, name_buf);
                strcat(name_buf, pathname);
                free(stat);
                if (name_buf[0] == '/') tok_id = 0;
                else tok_id = cwd;
                int id = resolve_path(name_buf, tok_id);
                free(name_buf);
                return id;
            } else {
                free(stat);
                return tok_id;
            }
        } else {
            free(stat);
            return -1;
        }
    }
}

int get_rec_id(int dir_id, char *name) {
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(dir_id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    int i = 0;
    int blockid = stat->blocks_map[0];
    while (blockid >= 0) {
        int read_offset = get_blocks_offset()
                          + blockid * h.block_size;

        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
        
        struct dir_rec *buffer = malloc(sizeof(struct dir_rec));
        
        int read_size = h.block_size;
        while (read_size > 0) {
            int rsize;
            if ((rsize=read(dev_id, buffer, sizeof(struct dir_rec))) < 0) {
                free(stat);
                free(buffer);
                return -READ_ERR;
            }
            
            if (0 == strcmp(name, buffer->name)) {
                int id = buffer->id;
                free(buffer);
                free(stat);
                return id;
            }
            read_size -= rsize;
        }
        free(buffer);
        blockid = stat->blocks_map[++i];
    }
    free(stat);
    return -1;
}

int rm_rec(int dir_id, char *name) {
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(dir_id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }
    int i = 0;
    int blockid = stat->blocks_map[0];
    while (blockid >= 0) {
        int read_offset = get_blocks_offset()
                          + blockid * h.block_size;

        if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
            free(stat);
            return -READ_ERR;
        }
        
        struct dir_rec *buffer = malloc(sizeof(struct dir_rec));
        
        int read_size = h.block_size;
        while (read_size > 0) {
            int rsize;
            if ((rsize=read(dev_id, buffer, sizeof(struct dir_rec))) < 0) {
                free(stat);
                free(buffer);
                return -READ_ERR;
            }
            
            if (0 == strcmp(name, buffer->name)) {
                struct dir_rec dirrec = {
                    .id = -1
                };
                for (int j = 0; j < MAX_NAMESIZE; j++)
                    dirrec.name[j] = 0;
                if (write_dir_rec(dir_id, &dirrec) < 0)
                    return -WRITE_ERR;
                free(stat);
                free(buffer);
                return 0;
            }
            read_size -= rsize;
        }
        free(buffer);
        blockid = stat->blocks_map[++i];
    }
    free(stat);
    return -1;
}

int get_sym_data(int sym_id, char *buffer) {
    struct fstat *stat = malloc(sizeof(struct fstat));
    if (vs_getstat(sym_id, stat) < 0) {
        free(stat);
        return -READ_ERR;
    }

    int blockid = stat->blocks_map[0];

    int read_offset = get_blocks_offset()
                        + blockid * h.block_size;

    if (lseek(dev_id, read_offset, SEEK_SET) < 0) {
        free(stat);
        return -READ_ERR;
    }

    if (read(dev_id, buffer, stat->size) < 0) {
        free(stat);
        free(buffer);
        return -READ_ERR;
    }

    free(stat);
    return 0;
}
