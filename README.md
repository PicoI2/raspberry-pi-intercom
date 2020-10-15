# raspberry-pi-intercom
All you need to make an intercom with raspberry pi.

How does it works
---------
This software configured in server mode, run on a raspberry pi, with at least a button, a microphone and a speaker.<br>
You can add a second raspberry pi in your house, with the software configured in client mode.<br>
When someone push the button, it send a notification to the client software and to your phone using pushsafer.<br>
You can speak with your guest and open him the door, either with the client raspberry pi, or from a web page with your phone.

Supported platforms
---------
- Raspbian 8.0 (Jessie)

Dependencies
---------
- Standard C++ librairies
- Boost
- Alsa
- websocketpp
- Jsoncpp
- ssl
- uuid

Compilation
---------
```Shell
sudo apt install cmake libboost-system-dev libasound2-dev libwebsocketpp-dev libjsoncpp-dev libssl-dev uuid-dev libspeexdsp-dev
git clone https://github.com/PicoI2/raspberry-pi-intercom
cd raspberry-pi-intercom
cmake .
make
```

How to use it
---------
Copy and modify the config-server.cfg or config-client.cfg file to match your configuration.<br>
If using port below 1024 dont forget to do : "sudo setcap 'cap_net_bind_service=+ep' rpi-intercom"<br>
Then run
```Shell
./rpi-intercom config-server.cfg
```

Configuration
---------
- mode (both)
server (outside) or client (inside)
- local-udp-port (both)
udp receive port for communicating with client/server
- client-ip (server)
- client-udp-port (server)
- http-port (both)
- pushsafer-key (server)
- pushsafer-device (server)
- input-ringbell (server)
ringbell button input
- output-door-open (server)
output to open door (connect it to a relay)
- output-audio-on (both)
output to enable audio
- sound-card-play (both)
aplay -l to list available cards
- sound-card-rec (both)
arecord -l to list available cards, could be 'default', 'plughw:1'
- password (both)
password to use web interface
- server-ip (client)
- server-udp-port (client)
- input-stop-ring (client)
- input-listen (client)
- input-speak (client)
- input-open-door (client)
- input-hangup (client)
- output-backlight-on (client)
output to put screen backlight on

HTTPS
---------
Uncomment "add_definitions(-DUSE_HTTPS)" in CMakeLists.txt<br>
Add your certificate files ssl/privkey1.pem and ssl/cert1.pem<br>
You can create your own SSL/TLS self-signed keys :<br>
```Shell
openssl req -x509 -newkey rsa:4096 -keyout ssl/privkey1.pem -out ssl/cert1.pem -days 365
```
But I recommand using a true certificate. You can buy domain name for 3,59 €/year on <a href="https://www.ovh.com/fr/domaines/dotovh.xml">OVH</a>.

Hardware
---------
As an example, this is a list of what I use :
- Raspberry Pi 3 B+ (~37€)
- Fisheye 5MP 1080P camera (~14€)
- USB 2.0 Microphone SF-555B (~8€)
- EU Plug 5V 3A converter micro USB (~7€)
- 2W 28mm speaker (~5€)
- LM386 Audio amplifier module 5-12V 10K (~3€)
- DC 5V Songle power relay (~1€)
- 3.5mm mono male plug jack adapter (~0.20€)
- 2N2222 transitor (~0.15€)


Build your own case with a 3d printer
---------
You could find in 3d-print directory files to print your own case.<br>
These files can be view with <a href="http://www.openscad.org/">OpenSCAD</a>
In variables.scad, you can replace "DOORBELL" by your family name. Check that your name do not overlap the button hole.<br>
Check or modifiy all diameter to suit your hardware.<br>
If the intercom will be placed outdoor, I recommend to print it in PetG (less sensitive to humidity and UV, and easier to print than ABS).

Camera (motion)
---------
To handle camera, use motion :
```Shell
sudo apt install motion
```
Enable camera (5 Interfacing options, P1 Camera):
```Shell
sudo raspi-config
```
Edit /etc/motion/motion.conf to change theses lines :
```Shell
daemon on
stream_localhost off
```

Make your camera motion compatible :
```Shell
sudo modprobe bcm2835-v4l2
echo "bcm2835-v4l2" | sudo tee -a /etc/modules
```

To start motion at startup, edit the file /etc/default/motion
```Shell
start_motion_daemon=yes
```

Pushsafer
---------
<a href="https://www.pushsafer.com/">https://www.pushsafer.com/</a>
Pushsafer allow you to receive notification on your phone.<br>
It's not free but cheap, with only 1$ you can receive 1000 notifications.<br>
But I would like to find another solution because it's not always very fast.<br>
