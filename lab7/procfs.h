#include "vfs.h"
#include "task.h"
#ifndef __PROCFS_H__
#define __PROCFS_H__


struct procfs_root_struct
{
  int switch_;
  struct vfs_vnode_struct * parent;
  /* hello and task info are queried on demand */
};

#define PROCFS_TYPE_SWITCH 0
#define PROCFS_TYPE_HELLO -1
/* the rest are task id */
/* /proc/1 -> internal == 1 */
/* /proc/1/status -> internal == 1 & PROCFS_TYPE_TASK_STATUS */
#define PROCFS_TYPE_TASK_STATUS 0x1000u

struct vfs_filesystem_struct * procfs_init(void);
int procfs_setup_mount(struct vfs_filesystem_struct * fs, struct vfs_mount_struct * mount);
int procfs_post_mount(struct vfs_vnode_struct * mountpoint_parent, struct vfs_mount_struct * mount);
int procfs_mount(struct vfs_vnode_struct * mountpoint_vnode, struct vfs_mount_struct * mount);
int procfs_umount(struct vfs_vnode_struct * mountpoint_parent, const char * mountpoint_token);
int procfs_lookup(struct vfs_vnode_struct * dir_node, struct vfs_vnode_struct ** target, const char * component_name);
int procfs_create(struct vfs_vnode_struct * dir_node, struct vfs_vnode_struct ** target, const char * component_name);
int procfs_write(struct vfs_file_struct * file, const void * buf, size_t len);
int procfs_read(struct vfs_file_struct * file, void * buf, size_t len);
int procfs_list(struct vfs_vnode_struct * dir_node);
int procfs_mkdir(struct vfs_vnode_struct * dir_node, const char * new_dir_name);

struct vfs_vnode_struct * procfs_create_vnode(struct vfs_mount_struct * mount, void * internal, int is_dir);

int procfs_task_status(char * string, int id);

extern struct task_struct * kernel_task_pool;

#endif

