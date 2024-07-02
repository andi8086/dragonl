
long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
ssize_t read(struct file *filp, char __user *buf, size_t len, loff_t *off);
ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
loff_t llseek(struct file *filp, loff_t off, int whence);
