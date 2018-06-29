/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/fs_struct.h>
#include "sgfs.h"
//#include "sgfs_lk.h"

static ssize_t sgfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = sgfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));

	return err;
}

static ssize_t sgfs_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	int err;

	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = sgfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
					file_inode(lower_file));
	}

	return err;
}


/*
static int  ecryptfs_filldir(struct dir_context *ctx, const char *lower_name, int lower_namelen, loff_t offset, u64 ino, unsigned int d_type)
 {
          struct ecryptfs_getdents_callback *buf = container_of(ctx, struct ecryptfs_getdents_callback, ctx);
          size_t name_size;
          char *name;
          int rc;
          
          buf->filldir_called++;
          rc = ecryptfs_decode_and_decrypt_filename(&name, &name_size,buf->sb, lower_name,lower_namelen);
          if (rc) {
//                  printk(KERN_ERR "%s: Error attempting to decode and decrypt ""filename [%s]; rc = [%d]\n", __func__, lower_name,rc);
                  goto out;
          }
          
          buf->caller->pos = buf->ctx.pos;
          rc = !dir_emit(buf->caller, name, name_size, ino, d_type);
          kfree(name);
          if (!rc)
                  buf->entries_written++;
  out:
          return rc;
  }
  
 
 static int ecryptfs_readdir(struct file *file, struct dir_context *ctx)
 {
         int rc;
         struct file *lower_file;
         struct inode *inode = file_inode(file);
         struct ecryptfs_getdents_callback buf = {
                 .ctx.actor = ecryptfs_filldir,
                 .caller = ctx,
                 .sb = inode->i_sb,
         };
         lower_file = ecryptfs_file_to_lower(file);
         rc = iterate_dir(lower_file, &buf.ctx);
         ctx->pos = buf.ctx.pos;
         if (rc < 0)
                 goto out;
         if (buf.filldir_called && !buf.entries_written)
                 goto out;
         if (rc >= 0)
                 fsstack_copy_attr_atime(inode,
                                        file_inode(lower_file));
 out:
         return rc;
 }

*/

void string_cat1(char *dest, char *src, char * result)
{
    int i = strlen(dest) ,j = strlen(src);
    char res[(i+j)];
    for (i = 0; dest[i] != '\0'; i++)
    {
        res[i]=dest[i];
    }
    for (j = 0; src[j] != '\0'; j++)
        res[i+j] = src[j];
    res[i+j] = '\0';
    memcpy(result,res,(i+j));
    return ;
}

static int sgfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

        //printk("\n uid : %d \n",dentry -> d_inode -> i_uid);
	lower_file = sgfs_lower_file(file);
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	return err;
}


int undelete_file( struct file *f, int args)
{   
    struct file *udel_file, *out_file;
    struct path cur_wd;
    char *filename = f -> f_path.dentry -> d_name.name, *fdpath = NULL, *fbuf, *file_str , *fls; 
    int err, f_len = sizeof(filename), key_size, fplen=PATH_MAX, e_flag=0;
//    char *key = (16,GFP_KERNEL);
    printk("\n_____________ undelete file name : %s _____________\n",filename);
    file_str = kmalloc(100,GFP_KERNEL);
    fbuf = kmalloc(100,GFP_KERNEL);
    if(strstr(filename,".enc"))
{
          printk("\n -----------------is enc------------\n");
          e_flag = 1;
}
//    file_str="/";
    //string_cat1("/",filename,file_str);
    get_fs_pwd(current -> fs, &cur_wd);
    //printk("\n$$$$$$$$$ cwd : %s  $$$$$$$$$$$$\n",cur_wd->dentry->d_name.name);
  
    fdpath = d_path(&(cur_wd),fbuf,fplen);
    strcpy(fbuf,fdpath);
    strcat(fbuf,"/");
    strcat(fbuf,filename);
  
    printk("\n__________________ path : %s ________________\n",fbuf);
    udel_file = filp_open ( fbuf, O_CREAT|O_WRONLY|O_RDONLY, 0644);
//    out_file = filp_open("/mnt/sgfs/.sg/out.txt" ,O_RDONLY, 0);

    err = read_decrypt_file(f, udel_file, 16,0);
    if (err)
    {
          printk("\n ERROR in read_encrypt \n");
          filp_close(udel_file, NULL);
          return err;
    }
    


    //path_put(&cur_wd);
//    kfree(fdpath);
//printk("\n_________vjkndkfv ________\n");
    kfree(file_str);
//printk("\n_________vjkndcxsvdsdvdskfv ________\n");
//kfree(fdpath);
    path_put(&cur_wd);
//    printk("\n time stamp : %d \n",(int)is_encypt(filename));
    //err = get_key_size_key(f,key_size,key);
   // printk("\n get key : %s \n",key);
 //   get_fs_pwd(current -> fs, cur_wd);
  //  printk("\n$$$$$$$$$ cwd : %s  $$$$$$$$$$$$\n",cur_wd->dentry->d_name.name);

  // kfree(key);

    //path_put(cur_wd);
 /*
 * get key and paading 
 * decrypt and move to pwd
 * if not .enc file the just move file to pwd  
   */
kfree(fbuf); 
filp_close(udel_file, NULL);
filp_close(out_file,NULL);
  



  return 0;
}


static long sgfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = sgfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
        switch(cmd)
        {
            case SGCTL_IOCTL_UNDELETE:err = undelete_file(file, arg);
                                      if(err)
                                           goto out;
                                      printk("undelete file\n");
                                      break;
        }
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

	/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
	if (!err)
		fsstack_copy_attr_all(file_inode(file),
				      file_inode(lower_file));
out:
	return err;
}

#ifdef CONFIG_COMPAT
static long sgfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = sgfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int sgfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = sgfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "sgfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!SGFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "sgfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &sgfs_vm_ops;

	file->f_mapping->a_ops = &sgfs_aops; /* set our aops */
	if (!SGFS_F(file)->lower_vm_ops) /* save for our ->fault */
		SGFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}

static int sgfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;

	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	file->private_data =
		kzalloc(sizeof(struct sgfs_file_info), GFP_KERNEL);
	if (!SGFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link sgfs's file struct to lower's */
	sgfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = sgfs_lower_file(file);
		if (lower_file) {
			sgfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		sgfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(SGFS_F(file));
	else
		fsstack_copy_attr_all(inode, sgfs_lower_inode(inode));
out_err:
	return err;
}

static int sgfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = sgfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int sgfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = sgfs_lower_file(file);
	if (lower_file) {
		sgfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(SGFS_F(file));
	return 0;
}

static int sgfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = sgfs_lower_file(file);
	sgfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	sgfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int sgfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = sgfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * Sgfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t sgfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = sgfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * Sgfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
sgfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = sgfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	return err;
}

/*
 * Sgfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
sgfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = sgfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
					file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations sgfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= sgfs_read,
	.write		= sgfs_write,
	.unlocked_ioctl	= sgfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sgfs_compat_ioctl,
#endif
	.mmap		= sgfs_mmap,
	.open		= sgfs_open,
	.flush		= sgfs_flush,
	.release	= sgfs_file_release,
	.fsync		= sgfs_fsync,
	.fasync		= sgfs_fasync,
	.read_iter	= sgfs_read_iter,
	.write_iter	= sgfs_write_iter,
};

/* trimmed directory options */
const struct file_operations sgfs_dir_fops = {
	.llseek		= sgfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= sgfs_readdir,
	.unlocked_ioctl	= sgfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sgfs_compat_ioctl,
#endif
	.open		= sgfs_open,
	.release	= sgfs_file_release,
	.flush		= sgfs_flush,
	.fsync		= sgfs_fsync,
	.fasync		= sgfs_fasync,
};
