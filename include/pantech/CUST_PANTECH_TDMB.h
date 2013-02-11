#ifndef __CUST_PANTECH_TDMB_H__
#define __CUST_PANTECH_TDMB_H__
/*
 2011/10/18 LS6 PANGSE
  내수와 해외를 구분하기 위해 FEATURE를 적용
*/
#if defined(T_EF45K) || defined(T_EF46L) || defined(T_EF47S)
  #define FEATURE_BUILD_WITH_TDMB
#endif

#endif /* __CUST_PANTECH_TDMB_H__ */