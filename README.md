# camsrvd

A basic IP security camera recording daemon with configurable motion detection, support for masking, and a web interface that supports camera live view as well as playback of recordings. Unlike some similar software this is designed to record continuously and highlight segments of video that contain motion using a heat map. Continuous recording eliminates the risk of motion-triggered recording which may not capture events that are important to you but did not contain enough motion to trigger a recording.

The project consists of the following parts:

1. `camsrvd` is the supervisory daemon for camera stream grabbing. It launches one instance of ffmpeg per camera which grab the video stream and save it to disk. It can detect if the ffmpeg instance(s) crash or terminate for some other reason and automatically restarts them. If an ffmpeg instance fails too many times in a row, it can disable just that instance from restarting. It also supports notifying you about problems with camera stream grabbing via sendmail.

2. `maintenance` is the maintenance program for camera recordings. This will perform motion detection and can optionally apply a mask to ignore certain parts of the video, like a busy public road. It will also delete recordings older than a certain number of days (configurable for each camera). It is designed to be run regularly (e.g. every 15 minutes) as a cron job and is smart enough to notice if a previous instance is still running because it is not finished yet, in which case it will exit silently.

3. `makemask` is the motion detection mask file creator and tester. Give this program a video file and it will save a bitmap of the first video frame which you can use as a starting guide for your motion mask. On the mask bitmap, make all areas to ignore black and all areas to consider white. If you pass a video file and a mask bitmap to this program, it will save a bitmap file of what motion detection will "see" once the mask is applied.

4. `htdocs` contains the PHP based web interface with the heat map. The web interface is in need of updating for more modern versions of PHP. It was first designed back in 2017 for PHP5 and should still work with PHP7.

Screenshot
----------
![Screenshot](https://i.imgur.com/zgAwLmu.png)

Required Debian Packages
------------------------

To run the binaries on a system:

```
# apt-get install ffmpeg libboost-filesystem1.55.0 libboost-program-options1.55.0 libboost-system1.55.0 libcv2.4 libopencv-calib3d2.4 libopencv-contrib2.4 libopencv-core2.4 libopencv-features2d2.4 libopencv-flann2.4 libopencv-gpu2.4 libopencv-highgui2.4 libopencv-imgproc2.4 libopencv-legacy2.4 libopencv-ml2.4 libopencv-objdetect2.4 libopencv-ocl2.4 libopencv-photo2.4 libopencv-stitching2.4 libopencv-superres2.4 libopencv-ts2.4 libopencv-video2.4 libopencv-videostab2.4`
```

To build from source, additionally:

```
apt-get install libboost-dev libboost-filesystem-dev libboost-program-options-dev libopencv-dev libopencv-video-dev cmake`
```

Build Instructions
------------------
Building from source is a piece of cake:

```
cmake CMakeLists.txt
make
```

There should be no errors. If there are errors it is most likely due to missing dependencies.

Installation Instructions
-------------------------

1. Put the compiled binaries in `/opt/camsrv`
1. Copy `etc/camsrv.ini` to `/etc` and edit the configuration to suit your needs
1. Copy `etc/camsrvd.initscript` to `/etc/init.d/camsrvd` and run `update-rc.d` accordingly
1. Copy `etc/camsrvd.cronjob` to `/etc/cron.d/camsrv`
1. Copy the `htdocs` folder to your webserver's public-facing directory
1. Start camsrvd and check the Syslog to see if it works

Setting it up is a bit fiddly at first, especially when working with motion masks, but once up and running it requires essentially no maintenance and will run in the background.

Tips
----
* How to undo motion detection results: `rename 's/-MOTION-[0-9]+.mp4$/.mp4/' *.mp4`

* Motion detection is extremely CPU intensive, so you will want to run this on a dedicated server with a powerful processor. The OpenCV-based motion detection algorithm should be able to detect multi-CPU setups and make good use of them.

* Recording many camera streams in parallel requires fast and durable hard disks. Do not use cheap or slow disk drives or there will be dropouts. WD Purple drives are known to work well.
