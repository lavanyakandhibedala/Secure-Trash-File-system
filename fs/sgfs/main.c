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

#include "sgfs.h"
#include <linux/module.h>

/*
 * There is no need to lock the sgfs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int sgfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;
	char *dev_name = (char *) raw_data;
	struct inode *inode;
        struct dentry *sg_dentry, *sg_parent_dentry;
        //struct encfiledata *k;  
//        sb -> s_root -> d_inode -> i_private = raw_data;

	if (!dev_name) {
		printk(KERN_ERR
		       "sgfs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		printk(KERN_ERR	"sgfs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}
 
	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct sgfs_sb_info), GFP_KERNEL);
	if (!SGFS_SB(sb)) {
		printk(KERN_CRIT "sgfs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	sgfs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &sgfs_sops;

	sb->s_export_op = &sgfs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = sgfs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &sgfs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	sgfs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */

	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO"sgfs: mounted on top of %s type %s\n",dev_name, lower_sb->s_type->name);


      /* 
        if ((char *)raw_data==NULL)
        {
             k->k_flag = 0;
             k->key="";
             k->key_size=0;     
        }
        else
        {
             if (sizeof((char *)raw_data)<16)
                 return -EINVAL;
             k->k_flag = 1;
             k->key=(char *)raw_data;
             k->key_size=sizeof((char *)raw_data);             
        }
        sb -> s_root -> d_inode -> i_private = raw_data;
       */ 


        sg_dentry = kern_path_create( AT_FDCWD , "/usr/src/hw2-lkandhibedal/hw2/mnt/sgfs/.sg", &lower_path ,1);
        done_path_create(&lower_path, sg_dentry);
    
        sg_parent_dentry = lock_parent(sg_dentry);
        err = vfs_mkdir(sg_parent_dentry->d_inode, sg_dentry, 0755);
	if (err)
        { 
  //              done_path_create(&lower_path, sg_dentry);
                unlock_dir(sg_parent_dentry);
		goto out;
        }
        unlock_dir(sg_parent_dentry);
//        done_path_create(&lower_path, sg_dentry);

        printk("\n .sg create done \n");

	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(SGFS_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	path_put(&lower_path);

out:
	return err;
}

struct dentry *sgfs_mount(struct file_system_type *fs_type, int flags,
			    const char *dev_name, void *raw_data)
{
	void *lower_path_name = (void *) dev_name;
        
        //struct dentry *D =mount_nodev(fs_type, flags, lower_path_name, sgfs_read_super); 
       // printk("\n $$$$   there at : %s\n%s $$$$$$\n",D->d_name.name,(char*)lower_path_name);

              


	return mount_nodev(fs_type, flags, lower_path_name,
			   sgfs_read_super);
}

static struct file_system_type sgfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= SGFS_NAME,
	.mount		= sgfs_mount,
	.kill_sb	= generic_shutdown_super,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(SGFS_NAME);

static int __init init_sgfs_fs(void)
{
	int err;

	pr_info("Registering sgfs " SGFS_VERSION "\n");

	err = sgfs_init_inode_cache();
	if (err)
		goto out;
	err = sgfs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&sgfs_fs_type);
out:
	if (err) {
		sgfs_destroy_inode_cache();
		sgfs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_sgfs_fs(void)
{
	sgfs_destroy_inode_cache();
	sgfs_destroy_dentry_cache();
	unregister_filesystem(&sgfs_fs_type);
	pr_info("Completed sgfs module unload\n");
}

MODULE_AUTHOR("Erez Zadok, Filesystems and Storage Lab, Stony Brook University"
	      " (http://www.fsl.cs.sunysb.edu/)");
MODULE_DESCRIPTION("Sgfs " SGFS_VERSION
		   " (http://sgfs.filesystems.org/)");
MODULE_LICENSE("GPL");

module_init(init_sgfs_fs);
module_exit(exit_sgfs_fs);
