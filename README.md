## CSMA/CA WLAN

Analyzing CSMA/CA (Carrier Sense Multiple Access with Collision Avoidance) Protocol in IEEE 802.11 WLAN protocol


## Requirements

The code is written in C++/C so G++/GCC compiler is required to run the code and simulation.

```
$ g++ simulator.cpp -o simulator
$ ./simulator
```

## Description

This project demonstrates the discrete event simulations of WLAN (Wireless Local Area Network) with single queue M/M/1 model. Each wireless host has a FIFO (First In First Out) queue which holds the data packets and communicates with other nodes through a shared channel. The length of data frames a negative exponentially distributed random variable in the range of 0 and 1544 bytes and acknowledgement frame is constant 64 bytes. The propagation delay is ignored and transmission rate, R is 11 msec.
