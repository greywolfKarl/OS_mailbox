#include "mailbox.h"

MODULE_LICENSE("Dual BSD/GPL");

static void get_process_name(char *ouput_name);
static ssize_t mailbox_read(struct kobject *kobj,
                            struct kobj_attribute *attr, char *buf);
static ssize_t mailbox_write(struct kobject *kobj,
                             struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobject *hw2_kobject;
static struct kobj_attribute mailbox_attribute
    = __ATTR(mailbox, 0660, mailbox_read, mailbox_write);
static struct list_head *ptr, *next;
static struct mailbox_head_t head;
static struct mailbox_entry_t* mail;
static struct mailbox_entry_t* curmail;
static int num_entry_max = 2;

module_param(num_entry_max, int, S_IRUGO);

static void get_process_name(char *ouput_name)
{
	memcpy(ouput_name, current->comm, sizeof(current->comm));
}

static ssize_t mailbox_read(struct kobject *kobj,
                            struct kobj_attribute *attr, char *buf)
{
	spin_lock(&head.sherlock);
	if(head.count <= 0)	{	//empty
		spin_unlock(&head.sherlock);
		return ERR_EMPTY;
	}
	list_for_each_safe(ptr, next, &head.head) {
		curmail = list_entry(ptr, struct mailbox_entry_t, entry);
		if(current->comm[0] == 's') {	//slave
			if(curmail->name[0] == 'm') {	//could only take mail from master
				printk(KERN_INFO "now %s reading\n", current->comm);
				printk(KERN_INFO "this mail is written by %s\n", curmail->name);
				sprintf(buf, "%s", curmail->data);
				printk(KERN_INFO "read buf: %s\n", buf);
				list_del(ptr);
				kfree(curmail);
				head.count--;
				printk(KERN_INFO "%d in list\n", head.count);
				spin_unlock(&head.sherlock);
				return strlen(buf);
			}
		} else {	//master
			if(curmail->name[0] == 's') {	//could only take mail from slave
				printk(KERN_INFO "now %s reading\n", current->comm);
				printk(KERN_INFO "this mail is written by %s\n", curmail->name);
				sprintf(buf, "%s", curmail->data);
				printk(KERN_INFO "read buf: %s\n", buf);
				list_del(ptr);
				kfree(curmail);
				head.count--;
				printk(KERN_INFO "%d in list\n", head.count);
				spin_unlock(&head.sherlock);
				return strlen(buf);
			}
		}
	}
	spin_unlock(&head.sherlock);
	return ERR_EMPTY;	//if all mail are written by self
}

static ssize_t mailbox_write(struct kobject *kobj,
                             struct kobj_attribute *attr, const char *buf, size_t count)
{
	spin_lock(&head.sherlock);
	if(current->comm[0] == 'm') {	//keep a space to prevent deadlock
		if(head.count >= num_entry_max-1) {
			spin_unlock(&head.sherlock);
			return ERR_FULL;
		}
	}
	if(head.count >= num_entry_max)	{	//full
		spin_unlock(&head.sherlock);
		return ERR_FULL;
	}
	mail = kmalloc(sizeof(*mail), GFP_KERNEL);
	list_add(&mail->entry, &head.head);
	head.count++;
	get_process_name(mail->name);
	printk(KERN_INFO "now %s writing\n", mail->name);
	printk(KERN_INFO "write buf: %s\n", buf);
	printk(KERN_INFO "%d in list\n", head.count);
	strcpy(mail->data, buf);
	spin_unlock(&head.sherlock);
	return head.count;
}

static int __init mailbox_init(void)
{
	printk("Insert\n");
	hw2_kobject = kobject_create_and_add("hw2", kernel_kobj);
	sysfs_create_file(hw2_kobject, &mailbox_attribute.attr);
	INIT_LIST_HEAD(&head.head);
	head.count = 0;
	spin_lock_init(&head.sherlock);//DEFINE_SPINLOCK() SPIN_LOCK_UNLOCKED;
	return 0;
}

static void __exit mailbox_exit(void)
{
	printk("Remove\n");
	kobject_put(hw2_kobject);
}

module_init(mailbox_init);
module_exit(mailbox_exit);
