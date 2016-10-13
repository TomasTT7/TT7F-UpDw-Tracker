# TT7F-UpDw-Tracker

This is the first working software for [TT7F high altitude balloon tracker](http://tt7hab.blogspot.cz/2016/08/the-tt7f-hardware.html).
It is intended for calssic up and down flights. The software runs continuous RTTY transmission in the 70cm band with an occasional APRS packet in the 2m band. The RTTY telemetry string conforms to the [UKHAS](https://ukhas.org.uk/communication:protocol) format to be displayed by the [HABHUB TRACKER](https://tracker.habhub.org). It doesn't implement any power saving routines and runs the SAM3S8B mcu on 64MHz PLL.

![alt tag](https://github.com/TomasTT7/TT7F-UpDw-Tracker/blob/master/TT7F-UpDw-tracker.jpg)
