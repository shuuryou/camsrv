# IP Camera Web Viewer (2025 Update)

## What This Project Was

Back in 2017, I built a comprehensive IP camera recording system with motion detection. IP cameras at the time streamed H.264, browsers could play H.264, life was simple. The project consisted of:

- **camsrvd**: A daemon that supervised ffmpeg processes for continuous recording
- **maintenance**: A motion detection system using OpenCV
- **makemask**: A tool for creating motion detection masks
- **Web interface**: A PHP-based viewer with heatmaps and playback

The philosophy was straightforward: record everything continuously, use motion detection during post-processing to highlight interesting segments. No missed events ever, because all of the footage is always available for manual review.

The old code is still available unchanged in `/legacy`, but it's unmaintained and you don't need it.

## What Changed

Around 2020-2021, camera manufacturers started switching to H.265/HEVC. Better compression, same quality, half the bandwidth - great for recording, terrible for web viewing. Browsers couldn't play H.265. The web interface became useless unless you wanted to transcode everything, which would melt your server.

Meanwhile:
- Modern cameras got sophisticated motion detection with object classification
- The maintenance tool with its basic three-frame-differencing motion detector is now redundant
- Lennart Poettering won and systemd has made custom supervisory daemons unnecessary

## Why This 2025 Update Exists

Because a bunch of IP camera manufacturers still ship garbage software.

I have some Chinese IP cameras that can only be managed with a sketchy piece of software called "VMS". These cameras stream RTSP, I needed web access for them, I had the old code available, and browsers finally started supporting H.265 in 2024/2025.

So I spent an hour working with Claude Opus 4 to modernize the PHP web interface, then spent a further hour checking for and fixing all of Claude's little mistakes and omissions. The result is:

- **PHP 8.4 compatibility**: Removed all deprecated features so the code will work for a few months before the PHP people decide to break compatibility again
- **Modern CSS**: Replaced Pure CSS with Pico CSS for mobile support
- **Updated VideoJS**: Now using v8.21.1
- **Cleaner JavaScript**: Modernized the client-side code
- **Better resilience**: Improved input validation and error handling


## What Works

- Viewing recordings in modern browsers (with H.265 support enabled)
- Basic navigation between recordings
- Downloading video recordings by right-clicking inside the video player to save the recording
- Live streaming, provided your camera provides a compatible stream
- Motion detection heatmaps, if you can provide the necessary data

## Setup Guide

### Prerequisites

On Debian 12 (Bookworm) or similar:

```bash
# Install Apache with PHP
apt install apache2 libapache2-mod-php8.4 php8.4-cli php8.4-common php8.4-mbstring

# Install ffmpeg for recording
apt install ffmpeg

# (Optional but recommended) Install mpm-itk for better security
apt install libapache2-mpm-itk
```

### Camera Recording with systemd

1. **Create user and directories**:
```bash
useradd -r -s /bin/false cameras
mkdir -p /mnt/cameras /etc/ffmpeg-cameras
chown cameras:cameras /mnt/cameras
```

2. **Create systemd service template** (`/etc/systemd/system/ffmpeg-cam@.service`):
```ini
[Unit]
Description=FFmpeg Stream Grabber %i
After=network-online.target
Wants=network-online.target

[Service]
Type=exec
EnvironmentFile=/etc/ffmpeg-cameras/%i.conf
SyslogIdentifier=%i-ffmpeg
RestartSec=30
Restart=always
User=cameras
Group=cameras

##################################################################
# NOTE                                                           #
# YOU NEED TO ADJUST THE FFMPEG COMMAND TO YOUR SPECIFIC CAMERA! #
# THIS COMMAND WILL PROBABLY NOT WORK OUT OF THE BOX!            #
##################################################################
ExecStart=/usr/bin/ffmpeg -v error -y -timeout 30000000 -use_wallclock_as_timestamps 1 -rtsp_transport tcp -i "rtsp://${CAMERA_IP}:554/user=admin&password=whatever&channel=1&stream=0.sdp?" -c:v copy -an -f segment -segment_format mp4 -segment_time 300 -reset_timestamps 1 -strftime 1 "/mnt/cameras/%i/%%Y%%m%%d_%%H%%M%%S.mp4"

[Install]
WantedBy=multi-user.target
```

3. **Create cleanup service** (`/etc/systemd/system/ffmpeg-cleanup.service`):
```ini
[Unit]
Description=FFmpeg recording cleanup
Wants=ffmpeg-cleanup.timer

[Service]
Type=oneshot
User=cameras
Group=cameras

# Adjust as necessary if you want to keep more than 5 days of recordings, etc.
ExecStart=/usr/bin/find /mnt/cameras/ -mindepth 1 -not -path '/mnt/cameras/lost+found/*' -not -path '/mnt/cameras/lost+found' -mtime +5 -delete -print
```

4. **Create cleanup timer** (`/etc/systemd/system/ffmpeg-cleanup.timer`):
```ini
[Unit]
Description=FFmpeg recording cleanup

[Timer]
Unit=ffmpeg-cleanup.service
OnCalendar=*-*-* 02:00:00
Persistent=true
AccuracySec=1h
RandomizedDelaySec=5m

[Install]
WantedBy=timers.target
```

5. **Configure each camera**:
```bash
mkdir -p /etc/ffmpeg-cameras/

# Adjust the IP addresses as required
echo "CAMERA_IP=192.168.1.201" >> /etc/ffmpeg-cameras/cam01.conf
echo "CAMERA_IP=192.168.1.202" >> /etc/ffmpeg-cameras/cam02.conf
echo "CAMERA_IP=192.168.1.203" >> /etc/ffmpeg-cameras/cam03.conf
```

6. **Enable services**:
```bash
# Enable cleanup timer
systemctl enable ffmpeg-cleanup.timer
systemctl start ffmpeg-cleanup.timer

# Enable cameras, for example:
for i in {01..03}; do
    systemctl enable ffmpeg-cam@cam$i.service
    systemctl start ffmpeg-cam@cam$i.service
done
```

### Web Interface Setup

1. **Configure Apache** (`/etc/apache2/sites-available/cameras.conf`):
```apache
<VirtualHost *:80>
    ServerName cameras.example.com
    DocumentRoot /var/www/cameras/
    
    #############################################################
    # IMPORTANT!                                                #
    # IT IS UP TO YOU TO ADD SOME SORT OF AUTHENTICATION SCHEME #
    #############################################################

    # Run as cameras user (requires mpm-itk)
    AssignUserId cameras cameras
    
    # Alias for video files
    Alias /video /mnt/cameras
    
    <Directory /var/www/cameras/>
        Options FollowSymLinks
        AllowOverride All
        Require all granted
    </Directory>
    
    <Directory /mnt/cameras/>
        Options -Indexes
        Require all granted
        
        <FilesMatch "\.mp4$">
            ForceType video/mp4
        </FilesMatch>
    </Directory>
</VirtualHost>
```

2. **Install web interface**:
```bash
cp -r htdocs/* /var/www/cameras/
chown -R cameras:cameras /var/www/cameras/
a2ensite cameras
systemctl reload apache2
```

3. **Configure** `/etc/camsrv.ini`:
```ini
; Remember to adjust the stream command and the stream URL to your specific camera.
; The samples won't work out of the box.

[webinterface]
cameras=cam01,cam02,cam03
title=Security Cameras
enablestream=1
enableheatmap=0
streamcommand=/usr/bin/ffmpeg -rtsp_transport tcp -i {STREAM} -c copy -an -movflags frag_keyframe+empty_moov -f mp4 -

[cam01]
title=Front Door
localurl=/video/cam01
stream=rtsp://192.168.1.201:554/user=admin&password=&channel=1&stream=0.sdp
destination=/mnt/cameras/cam01/

# ... repeat for other cameras
```

Now you can access the web interface inyour browser. If you get a blank page, check for PHP errors:

```bash
less /var/log/apache2/error.log
```

The repository includes a fully commented `camsrv.ini.sample` file that serves as a template for your configuration. For nostalgia's sake, the INI file format is compatible with the legacy components.


### Browser Configuration for H.265/HEVC

This is the crucial part that makes everything work with modern IP cameras:

**Firefox** (on Windows):
- Navigate to `about:config`
- Set `media.wmf.hevc.enabled` to `true`
- Restart Firefox

**Edge**: Works out of the box

**Chrome**: Hit or miss, depends on your OS

**Safari**: Broken

### Security Notes

1. **Always use HTTPS and basic auth** - The web interface has no built-in security
2. **Use mpm-itk** - Lets Apache run as the cameras user for `/mnt/cameras` access
3. **Firewall your cameras** - Never expose RTSP to the internet
4. **Separate network** - Ideally put cameras on their own VLAN

## Making Motion Detection Work Without the Old Maintenance Tool

If you want motion detection heatmaps in the web interface without using the legacy maintenance tool, you can rename files that contain motion to include a motion marker. The web interface looks for `-MOTION-[number]` before the file extension in the filename.

For example, if your camera detects motion and you want to highlight a recording with 30 units of motion:
```bash
mv 20250106_143000.mp4 20250106_143000-MOTION-30.mp4
```

You could integrate this with your camera's motion detection webhooks or ONVIF events to automatically rename files when motion occurs. It is up to you to decide what a unit of motion is; seconds, event count, whatever. It just has to be a number.

### Configuring the Heatmap

All you have to do is turn it on. The colors interpolate between values. If an hour has 300 units of motion, it'll blend between the 200 and 400 colors. Adjust the thresholds based on your environment - busy street? Use higher values. Quiet backyard? Lower them. The sample config has the same defaults that I thought were reasonable back in 2017 for some reason.

```ini
[webinterface]
enableheatmap=1

; Color gradient based on total motion seconds per hour
; Format: SECONDS:HEXCOLOR|SECONDS:HEXCOLOR|...
heatmapcolors=0:FFFFFF|200:F7B7B8|400:E1A0AD|600:BD5E71|800:BD3853|1000:BD0026
```

## The Reality

This is a "good enough" solution that was updated because I needed it. It's not actively maintained, but I fix it when it breaks for me and welcome patches. If this helps you escape some vendor's terrible software, great. If not, there are plenty of more sophisticated alternatives.

These days I primarily work with .NET Core. C♯ has always been my language of choice for most projects. I don't really like to work with PHP anymore, because both the tooling and the language feel barbaric.

Between work, family, and life in general, free time for projects like this is increasingly rare. These days, where possible, I just use Reolink cameras - their app actually works, doesn't require an account or push cloud features, includes a free relay service for remote access, and the hardware has been solid. Life is too short to fight with camera software, and Reolink has been refreshingly hassle-free.

## License

GNU General Public License v3.0 as before.