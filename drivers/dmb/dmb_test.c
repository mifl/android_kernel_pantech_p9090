//=============================================================================
// File       : dmb_test.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2012/03/29       yschoi         Create
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#include "dmb_comdef.h"
#include "dmb_type.h"
#include "dmb_test.h"


/*================================================================== */
/*==============        DMB TEST Definition     =============== */
/*================================================================== */

//#define FILENAME_PRE_DEMUX "/data/misc/dmb/pre_demux.txt"
//#define FILENAME_POST_DEMUX "/data/misc/dmb/post_demux.txt"
#define FILENAME_RAW_MSC "/data/misc/dmb/raw_msc.txt"
#define FILENAME_RAW_FIC "/data/misc/dmb/raw_fic.txt"



/*================================================================== */
/*==============        DMB TEST Function     =============== */
/*================================================================== */

void dmb_data_dump(u8 *p, u32 size, char *filename)
{
  struct file *file;
  loff_t pos = 0;
  int fd;
  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);
  
  fd = sys_open(filename, O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE, 0644);
  if(fd >= 0) 
  {
    file = fget(fd);
    if(file) 
    {
      vfs_write(file, p, size, &pos);
      fput(file);
    }
    sys_close(fd);
  }
  else
  {
    DMB_MSG_TEST("%s open failed  fd [%d]\n", __func__, fd);
  }
  set_fs(old_fs);
}

