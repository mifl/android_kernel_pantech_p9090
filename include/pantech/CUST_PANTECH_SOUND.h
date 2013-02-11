#ifndef __CUST_PANTECH_SOUND_H__
#define __CUST_PANTECH_SOUND_H__

#define FEATURE_SND_MODIFICATION // SangwonLee 110916


/* 
  2012/02/20 
  Qualcomm Patch feature
*/
#define FEATURE_PANTECH_SND_QCOM_CR

/* 
  2012/03/12
  Feature must be applied to all models
*/
#define FEATURE_PANTECH_SND

/*
 2011/10/24 [P11157]
*/
#if defined(T_EF44S)
#define FEATURE_PANTECH_SND_DOMESTIC
#define FEATURE_SKY_QSOUND_QFX
#define FEATURE_SKYSND_LPA  // for QSound LPA
#elif defined(T_MAGNUS) || defined(T_VEGAPVW) || defined(T_SIRIUSLTE)
#define FEATURE_PANTECH_SND_ABROAD
#elif defined(T_CSFB) || defined(T_SVLTE) // temp
#define FEATURE_PANTECH_SND_ABROAD  
#else
    #error "FEATURE_PANTECH_SND ? DOMESTIC or ABROAD"
#endif

#if defined(T_MAGNUS)
#define FEATURE_PANTECH_SND_MAGNUS

#define FEATURE_PANTECH_SND_REC_BACKMIC
#define FEATURE_PANTECH_SND_VOC_LOOPBACK
#define FEATURE_PANTECH_SND_BT_GROUPING
#endif

#if defined(T_VEGAPVW)
#define FEATURE_PANTECH_SND_VEGAPVW

#define FEATURE_PANTECH_SND_BT_ECNR
#define FEATURE_PANTECH_SND_BT_GROUPING
#define FEATURE_PANTECH_SND_ELECTOVOX

#define FEATURE_SKY_QSOUND_QFX
#define FEATURE_SKYSND_LPA  // for QSound LPA
#endif

#if defined(T_EF44S) || defined(T_MAGNUS) || defined(T_VEGAPVW)
#define FEATURE_PANTECH_SND_BOOT_SOUND
#endif

#endif /* __CUST_PANTECH_SOUND_H__ */
