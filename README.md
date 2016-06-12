# AM2321 Web server
This repo contains:
- C application to read AM2321 sensor (dir: am2321)
- Cron script to update values in result file (am2321/am2321-read.sh)
- php page for presenting read sensor values (dir: www)

# Requirments
Apache 2 with PHP 5.4 or newer. Raspberry Pi, LAN.
For building C application, you need gcc supporting C11 standard & i2c-dev.

# Apache2 server config
Add this to default virtualhost site in apache2:
```
Alias /sensor/ "/home/pi/am2321/www/"
<Directory "/home/pi/am2321/www/">
        Order allow,deny
        Allow from all
        # New directive needed in Apache 2.4.3:
        Require all granted
</Directory>
```

Add your user to www-data group (e.g.```sudo usermod -a -G www-data pi```)
chmod www & index.php:
```
chmod a+rx /home/pi/am2321/www /home/pi/am2321/www/index.php
```


# Cron
Add to cron (root user) this task:
```
* * * * * /home/pi/am2321/am2321/am2321-read.sh >/dev/null 2>&1
```

# Usage
After starting apache2, open in web browser address:
```
http://[server_ip_or_name]/sensor/
```

You can get XML output from script, to do this use this http link construction:
```
http://[server_ip_or_name]/sensor/?xml
```

