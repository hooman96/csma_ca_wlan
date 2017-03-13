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

## Implementation Logic

There are four types of events and their processing steps: 

1. Arrival
  * Generate the next arrival event
  * Create a data packet with random destination and current host as source
  * Insert the data packet to the current host queue
  
2. Departure  
  * Free the channel status
  * Create an acknowledegement packet and sends back to its host
  * Insert the acknowledegement packet to the current host queue

3. Sync
  * check the status of channel if the channel is busy, freeze the hosts backoff counters
  * if channel is free, decrement the counters
  * reached zero counter, create a new departure event to transmit the packet
  * while transmitting, create a new timeout event 
  * generate a new backoff counter for the current host (reset queue)
  * create next sync event and update sync time by adding .01 msec
  
4. Timeout 
  * Retransimission of the packet (failed transmiting), increase the transmission counter, n
  * set a new backoff in range of 0 and n * T
  
## Authors

* **Hooman Mo** - *Initial work* - [PurpleBooth](https://github.com/hooman96)
* **Allen Speers** [PurpleBooth](https://github.com/atspeers)
