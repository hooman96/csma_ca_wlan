# CSMA/CA WLAN

Analyzing CSMA/CA (Carrier Sense Multiple Access with Collision Avoidance) Protocol in IEEE 802.11 WLAN protocol


## Requirements

The code is written in 'C++/C' so 'G++/GCC' compiler is required to run the code and simulation.

```
$ g++ simulator.cpp -o simulator
$ ./simulator

$ g++ wirelessLan.cpp
$ ./a.out
```

## Description

This project demonstrates the discrete event simulations of WLAN (Wireless Local Area Network) with single queue M/M/1 model. Each wireless host has a FIFO (First In First Out) queue which holds the data packets and communicates with other nodes through a shared channel. The length of data frames a negative exponentially distributed random variable in the range of 0 and 1544 bytes and acknowledgement frame is constant 64 bytes. The propagation delay is ignored and transmission rate, R is 11 msec.

## Implementation Logic

There are four types of events and their processing steps: 

1. Arrival
  * Generate the next arrival event for the current host
  * Create a data packet with random destination and current host as source
  * Insert the data packet to the current host queue
  
2. Departure  
  * Free the channel status
  * Create an acknowledegement packet and sends back to its host
  * Insert the acknowledegement packet to the current host queue

3. Sync
  * check the status of channel if the channel is busy, freeze the hosts backoff counters
  * if channel is free, decrement the counters
  * reached zero counter, create a new departure event to transmit the packet, mark the channel busy
  * while transmitting, create a new timeout event 
  * generate a new backoff counter for the current host (reset queue)
  * create next sync event and update sync time by adding .01 msec
  
4. Timeout 
  * Retransimission of the packet (failed transmiting), increase the transmission counter, n
  * set a new backoff in range of 0 and n * T
  
## Data Analysis

Our simulations are runned variety of times with 1,000,000 events as default value. The plots are based on average delay of transmitted packets as function of lambda(arrival rate) as well as an additional plot which is average delay as function of T(maximum backoff values). We run the simulations with variety of T and lambda arrival rates packets/second and host numbers.
> T 20, 40, 80, 160, 320, 400 

> lambda values λ = 0.01, 0.05, 0.1, 0.3, 0.6, 0.8, 0.9


The throughput increases as lambda increases which makes sense since more packets are transmitted with higher rate. Average delay though depend on the congestion of the network and number of the hosts. Some of the data points might seem up and down but the reasoning is that number of simulations can affect the smaller lambda values. However, as lambda increases the data points express the simulations more clearly.


![alt tag](https://github.com/hooman96/csma_ca_wlan/blob/master/graph.png)

  
## Authors

* [**Hooman Mo**](https://github.com/hooman96)
* [**Allen Speers**](https://github.com/atspeers)
