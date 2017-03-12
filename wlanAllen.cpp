#include <iostream>
#include <list>
#include <string>
#include <queue>
#include <algorithm> // max
#include <cmath>
#include <math.h>




// randomly calculates negative-exponenetially-distributed-time
double nedt(double rate)
{
     double u;
     u = drand48();
     return ((-1/rate)*std::log(1-u));
}

class Event {
protected:
	double eventTime;

public:
	Event(double etime): eventTime(etime) {}

	double getEventTime() {
		return eventTime;
	}

    virtual void processEvent() = 0; // pure virtual function
};

class Arrival: public Event {
    int host;
public:
    Arrival(double etime, int h): Event(etime), host(h) {}

    void processEvent()
    {

    }
};

class Departure: public Event {
public:
    Departure(): Event(0) {}

    void processEvent(){}
};

class Sync: public Event {
public:
    Sync(double etime): Event(etime) {}

    void processEvent(){}
};

class Timeout: public Event{
public:
    Timeout(): Event(0) {}

    void processEvent(){}
};



class GEL { // Global Event List

	std::list<Event*> GlobalEventList;

public:
	GEL() {
        GlobalEventList = std::list<Event*>(); // this line may not be necessary, but oh well
    }

	void insert(Event *event) {
		if (GlobalEventList.size() == 0) {
            GlobalEventList.push_front(event);
            return;
        }

		for (std::list<Event*>::iterator itr = GlobalEventList.begin(); itr != GlobalEventList.end(); itr++) {
			if ((*itr)->getEventTime() > event->getEventTime()) {
                GlobalEventList.insert(itr, event);
                return;
            }
        }

        GlobalEventList.push_back(event);
	

	} // insert sorted by events time

	Event* removeFirst() {

		Event *firstElement = GlobalEventList.front();
		GlobalEventList.pop_front();

		return firstElement;
	}
};

// not sure i need all this stuff yet
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
    int backoff; // doing it in syc tics versus real time because easier for conflict avoidance
    int hostId;  // position in hosts array, maybe don't need this, but might if put create packets and stuff
    int tmNum; // transmission number, max tmNum = 1 + maxRTM.  Max backoff = tmNum * T
    std::queue<Packet> hostsQueue;

public:
    Host(){};
    Host(int id, double randomBackoffValue) {
        backoff = randomBackoffValue;
        hostsQueue = std::queue<Packet>();
        hostId = id;
        tmNum = 1;
    }

    double getBackOff() {
        return backoff;
    }
};



int	main(int argc, char const *argv[])
{

//  read from command line, but given default values
    double lambda = 0.1;    // dexcribes shape of arrival distribution
    double mu = 1;          // describes shape of distribution of PktSize (r)
    int N = 10;             // number of hosts in network.
    int T = 400;            // maximum backoff value in number sync cycles. Should be larger than N I suppose.
    double timeout = 0.005; // for project, will take values of 5, 10, or 15 msec. 
    int eventsSimulated = 100000; // the bound of for loop

//  these should be constant for our project, but I'll define them here
    double maxRTM = 3;              // maximum number of retransmissions
    int maxPktSize = 1544;          // maximum size of a packet in bytes
    int ackPktSize = 64;            // acknowledgement packet size in bytes
    double channelCapacity = 11000000.0;    // 11 Mbps (bits)
    double SIFS = 0.00005;                  // 0.05 msec, delay before ack
    double DIFS = 0.0001;                   // 0.1 msec, delay before send
    double SYNC = 0.00001;                  // 0.01 msec

//  these are variables used throught the program
    double time = 0;        // time simulated since start of simulation in seconds
    double transmitted = 0; // number of bytes successfully transmitted (will include ack bytes)
    double delay = 0;       // queue + transmission delay in seconds
    bool channelBusy = false; // true if channel is busy, false otherwise

//  containers
    Host* *hosts; // an array of host pointers
    GEL* eventList; // holds list of events

    // check if help option set (or if only 1 argument, since that would be invalid).
    //  If so, print help information and end program
    if ((argc > 1 && std::string("-help") == argv[1]) || argc == 2)
    {
        std::cout << "\nThis program simulates an IEEE 802.11-Based Wireless LAN. \n"
                     "To set parameters of network, use the following commands.\n"
                     "Default values are given in parenthesis.\n\n"
                     "-l: lambda(" << lambda << "), shape of arrival distribution\n"
                     "-m: mu(" << mu << "), shape of packet size distribution\n"
                     "-N: N(" << N << "), number of hosts on LAN\n"
                     "-T: T(" << T << "), maximum backoff time in sync cycles\n"
                     "-t: timeout(" << timeout << "), given in msec\n" 
                     "-s: eventsSimulated(" << eventsSimulated << "), controls length of simulation\n"<< std::endl;

        return 0;
    }

    // read in command line inputs, assume all inputs come in a -var val pair
    for (int i = 1; i + 1 < argc; i += 2)
    {
        // i probably should have made this a function, but i already did the copy paste so really no use now
        if (std::string("-N") == argv[i])
            try{
                N = std::stoi(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -N, using default value " << N << std::endl;
            }
        else if (std::string("-l") == argv[i])
            try{
                lambda = std::stod(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -l, using default value " << lambda << std::endl;
            }
        else if (std::string("-m") == argv[i])
            try{
                mu = std::stod(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -m, using default value " << mu << std::endl;
            }
        else if (std::string("-T") == argv[i])
            try{
                T = std::stoi(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -T, using default value " << T << std::endl;
            }
        else if (std::string("-t") == argv[i])
            try{
                timeout = std::stod(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -t, using default value " << timeout << std::endl;
            }
        else if (std::string("-s") == argv[i])
            try{
                eventsSimulated = std::stoi(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -s, using default value " << eventsSimulated << std::endl;
            }
        else
        {
            std::cout << "invalid option \"" << argv[i] <<"\".  To see valid options, run \"" << argv[0] << " -help\"" << std::endl;
        }
    }

    /*std::cout << "checking values:\n"
                         "-l: lambda(" << lambda << "), shape of arrival distribution\n"
                     "-m: mu(" << mu << "), shape of packet size distribution\n"
                     "-N: N(" << N << "), number of hosts on LAN\n"
                     "-T: T(" << T << "), maximum backoff time in sync cycles\n"
                     "-t: timeout(" << timeout << "), given in msec\n" << std::endl;
                     "-s: eventsSimulated(" << eventsSimulated << "), controls length of simulation\n" << std::endl;*/




    // Now the simulation can finally begin

    hosts = new Host*[N];           // create an array to hold all Hosts
    eventList = new GEL();          // create a list of events


    // initialize each host and create its initial arrival event
    for (int i = 0; i < N; i++)
    {
        hosts[i] = new Host();
        eventList->insert(new Arrival(time + nedt(lambda), i));

    }

    eventList->insert(new Sync(time + SYNC));  // create the first sync event


    for(int i = 0; i < eventsSimulated; i++)
    {
        
    }




    // delete dynamically allocated data
    for (int i = 0; i < N; i++)
    {
        delete hosts[i];
    }
    delete hosts;
    delete eventList;



}

        


/*


    Event e = Event(time + pareto(lambda), true);
    GEL eventList = GEL();
    eventList.insert(e);

    //cerr << "hello 1" << endl;

    // for 100000 events 
    // process event
    // just going by the number given

    for (int i = 0; i < eventsSimulated; i++)
    {
        // get closest event and update time
        e = eventList.removeFirst();

        // sums length by multiplying length by elapsed time
        // since length = 1 could still be considered empty queue
        // may want to chech it should be length, not length - 1
        sumLength += max(0, length - 1) * (e.getEventTime() - time);
        //cerr << "prev time: " << time  << "   event time: " << e.getEventTime() << endl; 

        // updates time
        time = e.getEventTime();

        // handles Arrival event
        if (e.getIsArrival())
        {
            //cerr << "is Arrival, i: " << i << endl;
            // insert new arrival event
            eventList.insert(Event(time + pareto(lambda), true));

            //cerr << "length: " << length << endl;

            // if server is free, schedule a departure event, and update length
            if (length == 0)
            {
                //cerr << "hello from length = 0" << endl;
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;
                busy += packet;
                eventList.insert(Event(time + packet, false));
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
        else 
        {
            //cerr << "is departure" << endl;
            length --;

            // if packet still in queue, create a departure event
            if (length > 0)
            {
                packet = nedt(mu);
                //cerr << "packet: " << packet << endl;

                busy += packet;
                eventList.insert(Event(time + packet, false));
            }
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
}*/



