# Linux IP Camera Recorder with Motion Detection and Web Interface

This is a basic IP security camera recording daemon with configurable motion detection, support for masking, and a basic web interface that supports camera live view as well as playback of recordings. Unlike some similar software this is designed to record continuously and highlight segments of video that contain motion using a heatmap.

Continuous recording eliminates a major risk of motion-triggered recording, which is that it may not capture events that are important to you, because they did not contain enough motion to trigger a recording. Disk space is cheap these days, so just capturing and keeping one or two weeks' worth of footage for review is feasible. Even if the motion detector provided here would completely fail to highlight what you were looking for, you could still find the event in the stored footage. For example, if your parked car got stolen, jump back to a point where it was still in the parking lot. Then jump forward a few hours. Is it gone? Jump back 30 minutes. Is it there again? Jump forward 10 minutes, etc. With a few clicks you will find the event.

If you need a highly configurable solution with "AI" and thousands of bells and whistles you can spend months fine-tuning, consider something like [ZoneMinder](https://github.com/ZoneMinder/zoneminder) instead. This project was designed to be a much simpler "set and forget" solution: set it up once, test it, and then forget all about it until you need to access the recorded footage.

The project consists of the following parts:

1. `camsrvd` is the supervisory daemon for camera stream grabbing. It launches one instance of ffmpeg per camera which grab the video stream and save it to disk. It can detect if the ffmpeg instance(s) crash or terminate for some other reason and automatically restarts them. If an ffmpeg instance fails too many times in a row, it can disable just that instance from restarting. It also supports notifying you about problems with camera stream grabbing via sendmail.

2. `maintenance` is the maintenance program for camera recordings. This will perform motion detection and can optionally apply a mask to ignore certain parts of the video, like a busy public road. It will also delete recordings older than a certain number of days (configurable for each camera). It is designed to be run regularly (e.g. every 15 minutes) as a cron job and is smart enough to notice if a previous instance is still running because it is not finished yet, in which case it will exit silently.

3. `makemask` is the motion detection mask file creator and tester. Give this program a video file and it will save a bitmap of the first video frame which you can use as a starting guide for your motion mask. On the mask bitmap, make all areas to ignore black and all areas to consider white. If you pass a video file and a mask bitmap to this program, it will save a bitmap file of what motion detection will "see" once the mask is applied.

4. `htdocs` contains the PHP based web interface with a heatmap, recording viewer, and live stream. It was first designed back in 2017 for PHP5 and updated in 2022 to have no errors or deprecation warnings with PHP 7.4 (see notes below). On the index page, a heatmap will group videos by hour and highlight the hours that contain motion. On the viewer, the individual videos containing motion are highlighted.

Screenshot
----------
![Screenshot](https://i.imgur.com/zgAwLmu.png)

Requirements
------------

To build from source:

```
apt install ffmpeg libboost-dev libboost-filesystem-dev libboost-program-options-dev libopencv-dev libopencv-video-dev cmake g++
```

This will pull in like 340 additional packages on a naked install. I am sorry.

You'll also need a web server with PHP set up and running.

The web interface requires mbstring:

```
apt install php-mbstring
```

To make notifications for continuously failing camera streams work, you need some program that offers a working `/usr/lib/sendmail -t`. That could be a local mail server, or something like `msmtp`.

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

This process works on [Devuan Chimaera](https://www.devuan.org/):

1. Put the compiled binaries in `/opt/camsrv`
1. Copy `etc/camsrv.ini` to `/etc` and edit the configuration to suit your needs
1. Copy `etc/camsrvd.initscript` to `/etc/init.d/camsrvd` and run `update-rc.d` accordingly
1. Copy `etc/camsrvd.cronjob` to `/etc/cron.d/camsrv`
1. Copy the `htdocs` folder to your webserver's public-facing directory
1. Start camsrvd and check the Syslog to see if it works

Here's a one-liner to do most of the above:

```
mkdir -p /opt/camsrv && cp bin/* /opt/camsrv/ && chmod +x /opt/camsrv/{camsrvd,maintenance,makemask} && cp etc/camsrv.ini /etc/ && cp etc/camsrvd.initscript /etc/init.d/camsrv && chmod +x /etc/init.d/camsrv && cp etc/camsrvd.cronjob /etc/cron.d/camsrv && update-rc.d camsrv defaults && update-rc.d camsrv enable
```

Setting it up is a bit fiddly at first, especially when working with motion masks, but once up and running it requires essentially no maintenance and will run in the background.

For Debian Linux with systemd, the process is largely the same, but you'll have to convert the init script to a service. Good luck.

The web interface needs to be secured via your web server's authentication settings, because it provides no security on its own. If you set up HTTPS and basic authentication, you'll have a secure web interface. 

The video folder should be accessible directly so your web server can stream the files directly. For Nginx, you can do it like this:

```
location /video/ {
    # Adjust path as necessary.
    alias /STORAGE/camera/;
    
    # Also check out the docs on improving streaming MP4 files:
    # http://nginx.org/en/docs/http/ngx_http_mp4_module.html
}
```

Notes
-----
* If the heatmap in the web interface only shows a blank row with a date, either motion detection is disabled (in that case you can turn the heatmap off in `/etc/camsrv.ini`), there was no motion at all during the entire day, or you need to dial down the motion detection sensitivity.

* How to undo motion detection results: `rename 's/-MOTION-[0-9]+.mp4$/.mp4/' *.mp4` (`rename` is not installed by default on most systems so you'll have to grab it via your package manager)

* How to test motion detection sensitivity: `/opt/camsrv/maintenance -c /etc/camsrv.ini -v` and look for the "MOTION AT" output.

* Got no IP cameras but still want to try running this? Here's a website offering a public RTSP test stream you could use for the `stream` and/or `livestream` settings in `/etc/camsrv.ini`: https://www.wowza.com/developer/rtsp-stream-test

* Motion detection with high video resolutions is extremely CPU intensive, so you will want to run this on a dedicated server with a powerful processor. The OpenCV-based motion detection algorithm should be able to detect multi-CPU setups and make good use of them. If it's still too slow, consider lowering the video resolution of your camera.

* Recording many camera streams in parallel requires fast and durable hard disks. Do not use cheap or slow disk drives or there will be dropouts. WD Purple drives are known to work well.

* Debian Bullseye and Devuan Chimaera provide PHP 7.4, so this is what the web interface is targeting. There may be minor incompatibilities with PHP 8, but it should be limited to basic things like changing a few `strftime` calls.


Debugging Motion Detection
--------------------------

Run `maintenance` with the `-v` option to make the motion detector output its state. The output is explained in the `maintenance.cpp` file, but here's a copy of the explanation:

```
/*
 * The verbose output here will be something like
 *
 * [...]
 * C0,C0,C0,C0,C0,C49,S1/3,C44,S2/3,C2,A,C64,S1/3,C67,S2/3,C9,A,
 * C2,C0,C0,C0,C0,C115,S1/3,C136,S2/3,C134,S3/3,
 * --- MOTION AT 127s ---
 * C19,A,C18,C1,C0,C0,
 * [...]
 *
 * The number after 'C' represents the number of changed pixels
 * between the previous, current, and next frame.
 *
 * If the 'C' number is greater than the "motionsensitivity"
 * setting, the program will start counting the frame sequence
 * ("motioncontinuation" setting) and you will get an 'S' with
 * two numbers; the first is the current number of frames that
 * had had enough changed pixels. The second is how many frames
 * with enough changes in pixels must occur in total before
 * whatever is happening is actually considered to be motion.
 *
 * Once 'S' matches (e.g. 'S3/3' above), you will receive a
 * "--- MOTION AT [...]s---" message with the video offset
 * in seconds where motion occurred. These are limited to one
 * message per second with motion.
 *
 * An 'A' means that motion has stopped; i.e. there were not
 * enough changed pixels anymore and therefore frame counting
 * was aborted.
 *
 * At the very the end, you get a log message like:
 *
 * INFO: Motion detection result for video file "[...]" was 18.
 *
 * The number at the end of this message indicates how many
 * times motion according to the configured parameters has
 * occurred in total for this video file.
 *
 * Unfortunately, for the time being, running this program in
 * verbose mode is the only way to test changes made to the
 * motion detection settings made in the "camsrv.ini" file.
 */
```
