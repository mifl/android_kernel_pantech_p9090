/************************************************************************************************
**
**    PANTECH AUDIO
**
**    FILE
**        pantech_audio.h
**
**    DESCRIPTION
**        This file contains pantech audio defines
**
**    Copyright (c) 2012 by PANTECH Incorporated.  All Rights Reserved.
*************************************************************************************************/

/************************************************************************************************
** Definition
*************************************************************************************************/
/* Default Register Value */ 

#define PANTECH_AUDIO_IOCTL_MAGIC		'z'
#define PANTECH_AUDIO_NR_OFF				_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 0, unsigned)
#define PANTECH_AUDIO_NR_ON				_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 1, unsigned)
#define PANTECH_AUDIO_NR_CTL				_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 2, unsigned)
#define PANTECH_AUDIO_AMP_GAIN			_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 3, unsigned) //subamp gaincoltrol p15994
#define PANTECH_AUDIO_AMP_MODE			_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 4, unsigned) //subamp mode info coltrol p14200
#define PANTECH_AUDIO_AMP_BE_SET			_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 5, unsigned) //subamp BE coltrol p14200
#define PANTECH_AUDIO_AMP_LOUD_SET		_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 6, unsigned) //subamp LOUD coltrol p14200
#define PANTECH_AUDIO_GET_AMP_GAIN 		_IOR(PANTECH_AUDIO_IOCTL_MAGIC, 7, unsigned) // subamp gaincontrol read
#define PANTECH_AUDIO_GET_MODEINFO 		_IOR(PANTECH_AUDIO_IOCTL_MAGIC, 8, unsigned) // subamp MODE_INFO read
#define PANTECH_AUDIO_GET_BE_SET 			_IOR(PANTECH_AUDIO_IOCTL_MAGIC, 9, unsigned) // subamp BE set read
#define PANTECH_AUDIO_GET_LOUD 			_IOR(PANTECH_AUDIO_IOCTL_MAGIC, 10, unsigned) // subamp Loud reg read


#define SND_AMP_IOCTL_MAGIC 'a'

// #define PANTECH_AUDIO_GET_AMP_GAIN _IOW(SND_AMP_IOCTL_MAGIC, 7, unsigned)



/*
#define PANTECH_AUDIO_NR_HANDSET_ON		_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 2, unsigned)
#define PANTECH_AUDIO_NR_SPEAKER_ON		_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 3, unsigned)
#define PANTECH_AUDIO_NR_HEADSET_ON		_IOW(PANTECH_AUDIO_IOCTL_MAGIC, 4, unsigned)
*/

/*=========================================================================*/

