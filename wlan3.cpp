#include <random> // random host
#include <iostream>
#include <list>
#include <queue>
#include <cmath>
#include <math.h>
#include <algorithm> // max

using namespace std;


// randomly calculates negative-exponenetially-distributed-time
double nedt(double rate);
double generateRandomBackOff(double t);
double randomDestination(int source);
double dataLengthFrame(double rate);
double transmissionTime(double r);

class Packet {
	int source;
	int destination;
	int packet_length;
	bool isAck; // true acknowledgement, false datapacket

public:
	Packet(int s, int dest, int packet_bytes, bool ack) {
		source = s;
		destination = dest;
		packet_length = packet_bytes;
		isAck = ack;
	}
};

class Host {
	double backoff;
	int hostId;
	std::queue<Packet> hostsQueue;

public:
	Host();
	Host(int id, double randomBackoffValue) {
		backoff = randomBackoffValue;
		hostsQueue = std::queue<Packet>();
		hostId = id;
	}

	double getBackOff() {
		return backoff;
	}
};

enum eventtype {
	arrival = 0, departure = 1, syncEvent = 2, timeout = 3
};

class Event {
	double eventTime;
	bool isArrival;
	eventtype eventType;

public:
	Event(double etime, eventtype event) {
		eventTime = etime;
		eventType = event;
	}

	double getEventTime() {
		return eventTime;
	}

	// bool getIsArrival() {
	// 	return arrival;
	// }

	eventtype getEventType() {
		return eventType;
	}

	bool operator==(const Event &rhs) const {
        return rhs.eventTime == eventTime;
    }

	bool operator>=(const Event &rhs) const {
        return rhs.eventTime >= eventTime;
    }

    bool operator>(const Event &rhs) const {
        return rhs.eventTime > eventTime;
    }
};


class GEL { // Global Event List

	std::list<Event> GlobalEventList;

public:
	GEL() {
        GlobalEventList = std::list<Event>();
    }
	
	void insert(Event event) {

		if (GlobalEventList.size() == 0) {
            GlobalEventList.push_front(event);
            return;
        }

		for (std::list<Event>::iterator itr = GlobalEventList.begin(); itr != GlobalEventList.end(); itr++) {
			if (itr->getEventTime() > event.getEventTime()) {
                GlobalEventList.insert(itr, event);
                return;
            }
        }

        GlobalEventList.push_back(event);
	
	} // insert sorted by events time

	Event removeFirst() {

        //cerr << "begin removeFirst" << endl;
		Event firstElement = GlobalEventList.front();
		GlobalEventList.pop_front();

        //cerr << "end removeFirst" << endl;

		return firstElement;
	}
};


int main(int argc, char const *argv[])
{
	// should be read in from command line
    double lambda;
    double mu;
    double maxbuffer;
    double sync;
    double T;

    //std::cout << "lambda: ";
    std::cin >> lambda;
    //std::cout << "mu: ";
    std::cin >> mu;
    //std::cout << "Buffer Size: ";
    std::cin >> maxbuffer;
    std::cin >> sync;
    std::cin >> T;

	// variables
    int length = 0;
    int dropNum = 0;
    double sumLength = 0;
    double time = 0;
    double busy = 0;
    double packet = 0;
    
    double r = 0; // data-length-frame
    int N = 10;
    int packetDestination = 0;
    int packetTransmissionTime = 0;
    // initalization
	GEL eventList = GEL();
	//Host *hosts = new Host[10];

	Host h = Host(0, 3); // id index of hosts array
	cout << h.getBackOff();

	std::queue<Packet> hostsQueue = std::queue<Packet>();

	// how to determine destination of packet? choose another random host
	// 0 index in hosts array
	packetDestination = randomDestination(1);
	r = dataLengthFrame(mu);
	Packet p = Packet(0, packetDestination, r, false);
	hostsQueue.push(p);
	packetTransmissionTime = transmissionTime(r);

	for(int i = 0; i < N; i++) 
	{
	    eventList.insert(Event(time + nedt(lambda), arrival));
	}
	eventList.insert(Event(time + nedt(sync), syncEvent));

	
	for (int i = 0; i < 100000; i++)
    {
        // get closest event and update time
        Event e = eventList.removeFirst();

        // sums length by multiplying length by elapsed time
        // since length = 1 could still be considered empty queue
        // may want to chech it should be length, not length - 1
        sumLength += max(0, length - 1) * (e.getEventTime() - time);
        //cerr << "prev time: " << time  << "   event time: " << e.getEventTime() << endl; 

        // updates time
        time = e.getEventTime();

        // handles Arrival event
        if (e.getEventType() == arrival)
        {
            // generate new arrival event
            eventList.insert(Event(time + nedt(lambda), arrival)); // arrival

            //cerr << "length: " << length << endl;

            // if server is free, schedule a departure event, and update length
            if (length == 0)
            {
                //cerr << "hello from length = 0" << endl;
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;
                busy += packet;
                eventList.insert(Event(time + packet, departure)); // departure
                length ++;
                // this assumes maxbuffer is at least one,
                // which is a good assumption because no buffer 
                // would have max buffer equal to 1
            }
            // else if room in buffer
            // maxbuffer = -1 denotes infinite buffer
            else if (maxbuffer == -1 || length - 1 < maxbuffer)
            {
                length ++;
                // handles generating of service time when departure event created
            }
            else // no room in buffer
            {
                dropNum ++;
            }
        }

        // handles departure event
        else if (e.getEventType() == departure)
        {
            //cerr << "is departure" << endl;
            length --;

            // if packet still in queue, create a departure event
            if (length > 0)
            {
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;

                busy += packet;
                eventList.insert(Event(time + packet, departure)); // departure
            }
        }

        else if (e.getEventType() == syncEvent) 
        {
        	eventList.insert(Event(time + packet, syncEvent)); 
        }
        
        else if (e.getEventType() == timeout) 
        {

        }

        else {
        	cerr << "Error : event type is not known!";
        }

    }

    std::cout << "lambda: ";
    std::cout << lambda << endl;

    std::cout << "mu: ";
    std::cout << mu << endl;

    std::cout << "Buffer Size: ";
    std::cout << maxbuffer << endl;
    
    cout << "Utilization: " << busy / time << endl;
    cout << "Mean queue length: " << sumLength / time << endl;
    cout << "Number of packets dropped: " << dropNum << endl << endl << endl;


	return 0;
}

double nedt(double rate)
{
     double u;
     u = drand48();
     return ((-1/rate)*log(1-u));
}

double generateRandomBackOff(double t)
{
	// cited http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0, 1);
	return int(t * dis(gen));
}

double randomDestination(int source)
{
	std::random_device rdev;
	std::mt19937 rgen(rdev());
	std::uniform_int_distribution<int> idist(0, 9); //(inclusive, inclusive)
	
	int rd = idist(rgen);
	// if random destination and source are same, recursively call the function!?
	if (rd == source) {
		randomDestination(source);  // should be rd = no?
	}
	return rd;
}

double dataLengthFrame(double rate) 
{
	// how? multiple by 1544 and round integer after calling nedt?
	// what is this time? for 1544 byte packet, (1544 × 8) / (11 × 106) = 1.12 msec

	return int(1544 * nedt(rate)); // prob wrong [0, 1544]
}

double transmissionTime(double r)
{
	return (r * 8) / (11 * pow(10,6));
}