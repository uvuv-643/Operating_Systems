#include "header.h"
#include "http.h"

#define ROOT_INO 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zinatulin Artem");
MODULE_VERSION("0.01");

void url_encode(const char *str, char *result, int len) {
    int i, j = 0;
    for (i = 0; i < len && str[i] != '\0'; ++i) {
        if ((str[i] >= 'A' && str[i] <= 'Z') ||
            (str[i] >= 'a' && str[i] <= 'z') ||
            (str[i] >= '0' && str[i] <= '9')) {
            result[j++] = str[i];
        } else {
            sprintf(result + j, "%%%02X", (unsigned char)str[i]);
            j += 3;
        }
    }
    result[j] = '\0';
}

int code_f(struct inode *parent_inode, struct dentry *child_dentry,
           const char *type, int flag) {
  ino_t root;
  struct inode *inode;
  const char *name = child_dentry->d_name.name;
  root = parent_inode->i_ino;
  ino_t http_ans = 0;
  char my_ino[11];
  snprintf(my_ino, 11, "%lu", root);
  char enc_str[256 * 3 + 1];
  printk(KERN_INFO "FIX STRING %s %s", name, enc_str);
  url_encode(name, enc_str, strlen(name) * 3);
  if (networkfs_http_call(token, "create", (ino_t)&http_ans, sizeof(ino_t), 3,
                          "parent", my_ino, "name", enc_str, "type", type)) {
    return -1;
  }
  inode = networkfs_get_inode(parent_inode->i_sb, NULL, flag, http_ans);
  // inode->i_op = &networkfs_inode_ops;
  // inode->i_fop = NULL;
  d_add(child_dentry, inode);
  return 0;
}

int empty_call(struct inode *parent_inode, struct dentry *child_dentry,
               const char *command) {
  const char *name = child_dentry->d_name.name;
  ino_t root;
  root = parent_inode->i_ino;
  ino_t http_ans = 0;
  char my_ino[11];
  snprintf(my_ino, 11, "%lu", root);
  char enc_str[256 * 3 + 1];
  url_encode(name, enc_str, strlen(name) * 3);

  if (networkfs_http_call(token, command, (ino_t)&http_ans, sizeof(ino_t), 2,
                          "parent", my_ino, "name", enc_str)) {
    return -1;
  }
  return 0;
}

struct dentry *networkfs_mount(struct file_system_type *fs_type, int flags,
                               const char *tkn, void *data) {
  strcpy(token, tkn);
  struct dentry *ret = mount_nodev(fs_type, flags, data, networkfs_fill_super);
  if (ret == NULL) {
    printk(KERN_ERR "Can't mount file system");
  } else {
    printk(KERN_INFO "Mounted successfuly");
  }
  return ret;
}

int networkfs_fill_super(struct super_block *sb, void *data, int silent) {
  struct inode *inode;
  inode = networkfs_get_inode(sb, NULL, S_IFDIR | 0777, ROOT_INO);
  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
    return -ENOMEM;
  }
  printk(KERN_INFO "return 0\n");
  return 0;
}

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *dir, umode_t mode,
                                  int i_ino) {
  struct inode *inode;
  inode = new_inode(sb);
  inode->i_op = &networkfs_inode_ops;
  inode->i_fop = &networkfs_dir_ops;
  inode->i_ino = i_ino;
  if (inode != NULL) {
    mode |= 511;
    inode_init_owner(&init_user_ns, inode, dir, mode);
  }
  return inode;
}

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  char fsname[256];
  struct dentry *dentry;
  struct inode *inode;
  unsigned long offset;
  int stored;
  unsigned char ftype;
  ino_t ino;
  ino_t dino;
  dentry = filp->f_path.dentry;
  inode = dentry->d_inode;
  offset = filp->f_pos;
  stored = 0;
  ino = inode->i_ino;

  entry_data http_ans = {};
  char my_ino[11];
  snprintf(my_ino, 11, "%lu", ino);
  if (networkfs_http_call(token, "list", (void *)&http_ans, sizeof(entry_data),
                          1, "inode", my_ino)) {
    return -1;
  }

  if (offset >= http_ans.entries_count) {
    return 0;
  }
  strcpy(fsname, ".");
  ftype = DT_DIR;
  dino = ino;
  offset += 1;
  ctx->pos += 1;
  dir_emit(ctx, ".", 1, dino, ftype);
  strcpy(fsname, "..");
  ftype = DT_DIR;
  dino = dentry->d_parent->d_inode->i_ino;
  offset += 1;
  ctx->pos += 1;
  dir_emit(ctx, "..", 2, dino, ftype);
  for (int i = 0; i < http_ans.entries_count; i++) {
    offset++;
    printk(KERN_INFO "DIR_EMIT %d", (int)http_ans.entries[i].entry_type);
    dir_emit(ctx, http_ans.entries[i].name, strlen(http_ans.entries[i].name),
             http_ans.entries[i].ino, http_ans.entries[i].entry_type);
  }
  // 1
  ctx->pos = offset;
  return http_ans.entries_count;
}

struct dentry *networkfs_lookup(struct inode *parent_inode,
                                struct dentry *child_dentry,
                                unsigned int flag) {
  ino_t root;
  struct inode *inode;
  const char *name = child_dentry->d_name.name;
  root = parent_inode->i_ino;
  entry_datat http_ans = {};
  char my_ino[11];
  snprintf(my_ino, 11, "%lu", root);
  char enc_str[256 * 3 + 1];
  url_encode(name, enc_str, strlen(name) * 3);
  if (networkfs_http_call(token, "lookup", (void *)&http_ans,
                          sizeof(entry_datat), 2, "parent", my_ino, "name",
                          enc_str)) {
    return NULL;
  }
  printk(KERN_INFO "Type %d %d", (int)(DT_REG), http_ans.entry_type);
  inode = networkfs_get_inode(parent_inode->i_sb, NULL,
                              http_ans.entry_type == DT_REG ? S_IFREG : S_IFDIR,
                              http_ans.ino);
  d_add(child_dentry, inode);
  return NULL;
}

int networkfs_create(struct user_namespace *fs_namespace,
                     struct inode *parent_inode, struct dentry *child_dentry,
                     umode_t mode, bool b) {
  return code_f(parent_inode, child_dentry, "file", S_IFREG | S_IRWXUGO);
}

int networkfs_mkdir(struct user_namespace *fs_name, struct inode *parent_inode,
                    struct dentry *child_dentry, umode_t mode) {
  return code_f(parent_inode, child_dentry, "directory", S_IFDIR | S_IRWXUGO);
}

int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry) {
  return empty_call(parent_inode, child_dentry, "unlink");
}

int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry) {
  return empty_call(parent_inode, child_dentry, "rmdir");
}

int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir,
                   struct dentry *new_dentry) {
  int errcode = 0;
  const char parent_ino[11], source_ino[11];
  snprintf(parent_ino, 11, "%lu", parent_dir->i_ino);
  snprintf(source_ino, 11, "%lu", old_dentry->d_inode->i_ino);
  return networkfs_http_call(token, "link", (int)&errcode, sizeof(int), 3,
                             "source", source_ino, "parent", parent_ino, "name",
                             new_dentry->d_name.name);
}

ssize_t networkfs_read(struct file *filp, char *buffer, size_t len,
                       loff_t *offset) {
  struct dentry *dentry;
  struct inode *inode;
  ino_t ino;
  dentry = filp->f_path.dentry;
  inode = dentry->d_inode;
  ino = inode->i_ino;
  char ino_node[10];
  snprintf(ino_node, 10, "%lu", ino);
  struct content *content;
  if ((content = kzalloc(sizeof(struct content), GFP_KERNEL)) == NULL) {
    return 0;
  }
  int status =
      networkfs_http_call(token, "read", (char *)content,
                          sizeof(struct content), 1, "inode", ino_node);
  char *text;
  if ((text = kzalloc(content->content_length, GFP_KERNEL)) == NULL) {
    return 0;
  }

   long rest = 0;
  if (len + *offset < content->content_length) {
    rest = len;
  } else {
    rest = content->content_length - *offset;
  }

  strcpy(text, content->content);  
  int wrote = copy_to_user(buffer, text, content->content_length);

  if (wrote != 0) {
    printk(KERN_INFO "can't write\n");
    kfree(content);
    return wrote;
  }

  kfree(content);
  (*offset) += (rest - wrote);
  return rest - wrote;

}

char *parse_symbols(const char *str) {
  char *new_str;
  if ((new_str = kzalloc(3 * strlen(str), GFP_KERNEL)) == NULL) {
    return 0;
  }
  for (int i = 0, j = 0; i < strlen(str); i++) {
    printk(KERN_INFO "%c\n", str[i]);
    new_str[j++] = '%';
    new_str[j++] = str[i] / 16 + '0';
    if (str[i] % 16 >= 10) {
      new_str[j++] = (str[i] % 16 - 10) + 'A';
    } else {
      new_str[j++] = str[i] % 16 + '0';
    }
  }
  printk(KERN_INFO "%s\n", new_str);
  return new_str;
}

ssize_t networkfs_write(struct file *filp, const char *buffer, size_t len,
                        loff_t *offset) {
  struct dentry *dentry;
  struct inode *inode;
  ino_t ino;
  dentry = filp->f_path.dentry;
  inode = dentry->d_inode;
  ino = inode->i_ino;
  char ino_node[10];
  snprintf(ino_node, 10, "%lu", ino);
  ino_t write_ino;
  char *text;
  printk(KERN_INFO "TRY TO KZALLOC");
  if ((text = kzalloc(len, GFP_KERNEL)) == NULL) {
    return 0;
  }
  printk(KERN_INFO "TRY TO COPY FROM USER");
  copy_from_user(text, buffer, len);
  printk(KERN_INFO "TRY TO SEND REQUEST %s", parse_symbols(text));
  int status = networkfs_http_call(
      token, "write", (char *)&write_ino, sizeof(write_ino), 2,
      "inode", ino_node, "content", parse_symbols(text));
  offset += len;
  kfree(text);
  return len;
}

void networkfs_kill_sb(struct super_block *sb) {
  printk(KERN_INFO
         "networkfs super block is destroyed. Unmount successfully.\n");
}

int networkfs_init(void) {
  if (register_filesystem(&networkfs_fs_type)) {
    printk(KERN_INFO "Cant init networkfs!\n");
    return 1;
  }
  printk(KERN_INFO "Hello, World!\n");
  return 0;
}

void networkfs_exit(void) {
  unregister_filesystem(&networkfs_fs_type);
  printk(KERN_INFO "Goodbye!\n");
}

module_init(networkfs_init);
module_exit(networkfs_exit);