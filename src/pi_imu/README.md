# Install Raspbian on the Raspberry PI

- Baixe a imagem do sistema RASPBIAN STRETCH WITH DESKTOP no site do raspberry (https://www.raspberrypi.org/downloads/raspbian/)
Obs: Nao baixe o arquivo LITE pois este possui apenas interface por linha de comando.

- Será necessário identificar o nome do dispositivo do cartão SD. Para isto, antes de inserir o cartão de memória execute o seguinte comando para ver as unidades de disco presentes.

```bash
 $ sudo fdisk -l
 ou
 $ ls -la /dev/sd*
```
- Insira o cartão e execute novamente o comando de forma a identificar o nome do cartao de SD que irá aparecer. Será algo do tipo /dev/sd...

```bash
 $ sudo fdisk -l
```

- Caso o cartão SD apareça como por exemplo /dev/sde1 ou /dev/sde2, eliminar o numero e considerar apenas /dev/sde 

- Execute o seguinte comando para copiar a imagem do Raspibian para o cartão SD fazendo as alterações de caminho necessárias.

```bash
 $ sudo dd if=/home/usr/Downloads/2018-04-18-raspbian-stretch.img of=/dev/sd...
```

- O cartão esta formatado e pode ser inserido no Raspberry para utilização.


# Enable the IMU

- Não execute upgrade

```bash
 $ sudo raspi-config
```


# How to Enable i2c on the Raspberry Pi

```bash
$ sudo nano /etc/modprobe.d/raspi-blacklist.conf

Place a hash '#' in front of blacklist i2c-bcm2708
If the above file is blank or doesn't exist, then skip the above step


$ sudo nano /etc/modules
Add these two lines;

i2c-dev
i2c-bcm2708

$ sudo nano /boot/config.txt

Add these two lines to the bottom of the file:
dtparam=i2c_arm=on
dtparam=i2c1=on

$ sudo reboot

Once your Raspberry Pi reboots, you can check for any components connected to the i2c bus by using i2cdetect;

$ sudo /usr/sbin/i2cdetect -y 1

A table like the table below will be shown and if any divices are connected, thier address will be shown. 
Below you can see that a device is connected to the i2c bus which is using the address of 0x6b.

0 1 2 3 4 5 6 7 8 9 a b c d e f
00: -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- 6b -- -- -- --
70: -- -- -- -- -- -- -- --
```
# Install Dependencies

```bash
 $ sudo apt-get update
 $ sudo apt-get install i2c-tools libi2c-dev python-smbus
 $ sudo apt-get install liboctave-dev
 $ sudo apt-get install subversion
 ```

# Install carmen_lcad sources

 $ svn checkout https://github.com/LCAD-UFES/carmen_lcad/trunk/src/ ~/carmen_lcad/src
 $ svn checkout https://github.com/LCAD-UFES/carmen_lcad/trunk/sharedlib/libcmt/ ~/carmen_lcad/sharedlib/libcmt
```

- Baixe e compile uma versão mais atual do IPC

```bash
 $ cd /usr/local
 $ sudo wget http://www.cs.cmu.edu/afs/cs/project/TCA/ftp/ipc-3.9.1a.tar.gz
 $ sudo tar -xzvf ipc-3.9.1a.tar.gz
 $ cd ipc-3.9.1/src/
 $ sudo cp ~/carmen_lcad/src/xsens_MTi-G/formatters.h .
 $ make
```

- Substitua o arquivo Makefile.rules do src do carmen

```bash
 $ cp ~/carmen_lcad/src/xsens_MTi-G/Makefile.rules ~/carmen_lcad/src/
```

# Configure CARMEN LCAD

```bash
 $ cd ~/carmen_lcad/src
 $ ./configure --nojava --nozlib --nocuda
 Should the C++ tools be installed for CARMEN: [Y/n] Y
 Should Python Bindings be installed: [y/N] N
 Should the old laser server be used instead of the new one: [y/N] N
 Install path [/usr/local/]: 
 Robot numbers [*]: 1,2
```

# Compile and executing the pi_imu drive module on the Raspberry PI

```bash
 $ 
 $ cd ~/carmen_lcad/src/pi_imu/RTIMULib2/Linux
 $ mkdir build
 $ cd build
 $ cmake ..
 $ make
 $ cd ~/carmen_lcad/src/pi_imu/pi_imu_server
 $ make
 $ ./Output/pi_imu_server_driver 
```

# Testing and calibrating the pi_imu drive module on the Raspberry PI

```bash
 $ cd ~/carmen_lcad/src/pi_imu/RTIMULib2
 $ ./Linux/build/RTIMULibDemoGL/RTIMULibDemoGL {or ./Linux/build/RTIMULibDemo/RTIMULibDemo if OpenGL is not working}
 ```
For calibration see ~/carmen_lcad/src/pi_imu/RTIMULib2/Calibration.pdf

# Compile the pi_imu client drive module on your computer

```bash
 $ cd $CARMEN_HOME/src/pi_imu
 $ make
 $ ./pi_imu_client_driver
```

# Visualize the pi_imu module on your computer

```bash
 $ cd $CARMEN_HOME/src/pi_imu_viewer
 $ make
 $ $CARMEN_HOME/bin/imu_viewer 
```

 The pi_imu_viewer program will display the image of a box representing the state of the IMU


# Configure an Static IP to the Raspberry PI on IARA's network
 
 Start by editing the dhcpcd.conf file
 
```bash
 $ sudo nano /etc/dhcpcd.conf
```

 Add at the end of the file the following configuration:
 eth0 = wired connection
 For the first pi_camera we used the IP adress 192.168.0.15/24
 Replace the 15 with the IP number desired
 Make sure you leave the /24 at the end of the adress
 
```ini
 interface eth0

 static ip_address=192.168.1.15/24
 static routers=192.168.1.1
 static domain_name_servers=8.8.8.8
```

 To exit the editor, press ctrl+x
 To save your changes press the letter “Y” then hit enter
 
 Reboot the Raspberry PI
 
```bash
 $ sudo reboot
```

 You can double check by typing:
 
```bash
 $ ifconfig
```

 The eth0 inet addr must be 192.168.1.15
