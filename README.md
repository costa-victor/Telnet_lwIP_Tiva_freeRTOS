# Telnet_lwIP_Tiva_freeRTOS
Telnet Server project based on lwIP and freeRTOS+CLI for Tiva Connected TM4C1294XL.

## Screenshots

![gif7](./images/project7.gif)


## Requirements

The project was developed on **Code Composer Studio Version: 10.1.0.00010**

The CCS Project was created and configurated exactly like the images below:

![img1](./images/project1.png)

Change here **/home/victor/ti/tivaware** to your **tivaware** path
![img2](./images/project2.png)

Also here...
![img3](./images/project3.png)


That's all !

## How to use
Run the commmand:

```bash
victor@victorUTF:~$ dmesg | grep tty
  [    0.090095] printk: console [tty0] enabled
  [ 1950.239035] cdc_acm 1-1:1.0: ttyACM0: USB ACM device
  [32232.341969] cdc_acm 1-1:1.0: ttyACM0: USB ACM device
```
Connect in **dev/ttyACM0** serial port, using **GtkTerm**, **PuTTy** or similars, and set port to **115200 8-N-1**.

Check the **IP Address** on the serial port to find out which IP to connect.
![img4](./images/project4.png)

And finally, connect to the telnet server.
![img5](./images/project5.png)
![img6](./images/project6.png)

