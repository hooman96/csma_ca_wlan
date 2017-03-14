#include <random> // random generators
#include <iostream>
#include <list>
#include <queue>
#include <cmath>
#include <math.h>
#include <algorithm> // max

using namespace std;


double nedt(double rate); // randomly calculates negative-exponenetially-distributed-time
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
public:
	double backoff;
	int hostId;
	std::queue<Packet> hostQueue;

	Host();
	Host(int id, double randomBackoffValue) {
		backoff = randomBackoffValue;
		hostQueue = std::queue<Packet>();
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
	int eventHostId;

public:
	Event(double etime, eventtype event, int hostId) {
		eventTime = etime;
		eventType = event;
		eventHostId = hostId;
	}

	double getEventTime() {
		return eventTime;
	}

	int getHost() {
		return eventHostId;
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

		Event firstElement = GlobalEventList.front();
		GlobalEventList.pop_front();

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
    double timeoutTime;
    // double timeout; // 5 10 15

    //std::cout << "lambda: ";
    std::cin >> lambda;
    //std::cout << "mu: ";
    std::cin >> mu;
    //std::cout << "Buffer Size: ";
    std::cin >> maxbuffer;
    std::cin >> sync;
    std::cin >> T;
    std::cin >> timeoutTime;

	// variables
    int length = 0;
    int dropNum = 0;
    double sumLength = 0;
    double time = 0;
    double busy = 0;
    double packet = 0;
    
    bool channelBusy = false; // false free, busy true
    double r = 0; // data-length-frame
    int N = 10;
    int packetDestination = 0;
    int packetTransmissionTime = 0;
    int n = 0; // transmission count

	int acknowledgementTime = 0;
	int sendingTime = 0;
	int sifs = .05 * pow(10, 3);
	int difs = .1 * pow(10, 3);


    // initalization
	GEL eventList = GEL();

	// Host *hosts = new Host[N];
	std::vector<Host> hosts;
	
	for (int i = 0; i < N; ++i)
	{
		hosts.push_back( Host(i, generateRandomBackOff(T)) );
	}

	for(int i = 0; i < N; i++) 
	{
	    eventList.insert(Event(time + nedt(lambda), arrival, i));
	}
	eventList.insert(Event(time + nedt(sync), syncEvent, -1)); // sync events do not have host, code -1
	
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
            eventList.insert(Event(time + nedt(lambda), arrival, e.getHost())); // arrival

			r = dataLengthFrame(mu);

        	packetDestination = randomDestination(e.getHost());
        	Packet p = Packet(e.getHost(), packetDestination, r, false); // data packet for arrivals

            // insert packet to queue
			hosts[e.getHost()].hostQueue.push(p);

            // if server is free, schedule a departure event, and update length
            if (length == 0 || hosts[0].hostQueue.size() == 0)
            {
                //cerr << "hello from length = 0" << endl;
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;
                busy += packet;
                eventList.insert(Event(time + packet, departure, e.getHost())); // departure
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
        	channelBusy = false; // free the channel 

        	packetDestination = randomDestination(e.getHost());
        	Packet ap = Packet(e.getHost(), packetDestination, 64, true); // acknowledgement packet for depatures

        	hosts[e.getHost()].hostQueue.push(ap);

            length --;

            // if packet still in queue, create a departure event
            if (length > 0)
            {
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;

                busy += packet;
                eventList.insert(Event(time + packet, departure, e.getHost())); // departure
            }
        }

        else if (e.getEventType() == syncEvent) 
        {
        	// decreament counter for free channel
        	if (channelBusy == false) {

        		hosts[e.getHost()].backoff--;

        		if (hosts[e.getHost()].getBackOff() == 0) { // create departure event
        			eventList.insert(Event(time + packet, departure, e.getHost())); // departure event
        			channelBusy = true;

        			eventList.insert(Event(time + packet, timeout, e.getHost())); // timeout event while transmiting
        			hosts[e.getHost()].backoff = generateRandomBackOff(T); // generate new random backoff value
        		}
        	}

        	eventList.insert(Event(time + packet, syncEvent, e.getHost())); // next synchrinization event
        	sync += (.01 * pow(10, 3)); // msec to sec
        }
        
        else if (e.getEventType() == timeout) 
        {
            eventList.insert(Event(time + timeout, departure, e.getHost())); // initial departure 


            acknowledgementTime = e.getEventTime() + transmissionTime(64) + sifs;
        	// sendingTime = time + transmissionTime(r) + difs;

            
    		// failed transmission : resend the packet, increase transmission count n
            n++;
            hosts[e.getHost()].backoff = generateRandomBackOff(T * n);                
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
		rd = randomDestination(source);
	}
	return rd;
}

double dataLengthFrame(double rate) 
{
    // http://en.cppreference.com/w/cpp/numeric/random/exponential_distribution
    std::random_device rd;
    std::mt19937 gen(rd());

    std::exponential_distribution<> d(1); // generate nedt between 0 and 1

    return int(1544 * d(gen));
}

double transmissionTime(double r)
{
	return (r * 8) / (11 * pow(10,6));
}