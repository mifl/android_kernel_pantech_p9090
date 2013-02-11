//=============================================================================
// File       : dmb_test.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2012/03/29       yschoi         Create
//=============================================================================

#ifndef _DMB_TEST_H_
#define _DMB_TEST_H_

#include "dmb_type.h"


/* ========== Message ID for DMB ========== */

#define DMB_MSG_TEST(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



void dmb_data_dump(u8 *p, u32 size, char *filename);

#endif /* _DMB_TEST_H_ */
