#include <iostream>
#include <list>
#include <string>
#include <queue>
#include <algorithm> // max
#include <cmath>
#include <math.h>
#include <random> // for hooman random stuff





// randomly calculates negative-exponenetially-distributed-time
double nedt(double rate)
{
     double u;
     u = drand48();
     return ((-1/rate)*std::log(1-u));
}

// generate a random backoff value less than or equal to T that is not currently in the backoff list
int generateRandomBackOff(int T, const int backoff[], int N)
{
    // cited http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

    std::random_device rdev;
    std::mt19937 rgen(rdev());
    std::uniform_int_distribution<int> idist(1, T); //(inclusive, inclusive)
    
    int rd = idist(rgen);

    // make sure the backoff value is not already given to another node
    // this is for collision avoidance as the TA told us to do, even though 
    // it doesn't really happen in real life.
    for (int i = 0; i < N; i++)
    {
        if (rd == backoff[i])
        {
            return generateRandomBackOff(T, backoff, N);
        }
    }
    return rd;
}

int randomDestination(int source, int N)
{
    // cited http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

    std::random_device rdev;
    std::mt19937 rgen(rdev());
    std::uniform_int_distribution<int> idist(0, N - 1); //(inclusive, inclusive)
    
    int rd = idist(rgen);
    // if random destination and source are same, recursively call the function!?
    if (rd == source) {
        rd = randomDestination(source, N);
    }
    return rd;
}

enum eventtype {
    arrival, departure, sync, timeout
};


class Event {
protected:
	double eventTime;
    eventtype type;


public:
	Event(eventtype type, double etime): type(type), eventTime(etime) {}
    virtual ~Event(){}

	double getEventTime() {
		return eventTime;
	}

    eventtype getType()
    {
        return type;
    }

    virtual void processEvent() = 0; // pure virtual function

};

class Arrival: public Event {
    static double lambda;

    int host;

public:
    Arrival(double stime, int h): Event(arrival, stime + nedt(lambda)), host(h) {}

    static void setLambda(double l)
    {
        lambda = l;
    }

    int getHost()
    {
        return host;
    }
    void processEvent()
    {

    }
};

double Arrival::lambda = 0;

class Departure: public Event {
public:
    Departure(): Event(departure, 0) {}

    void processEvent(){}
};

class Sync: public Event {

    static double SYNC;

public:
    Sync(double stime): Event(sync, stime + SYNC) {}

    static void setSYNC(double s)
    {
        SYNC = s;
    }

    void processEvent(){}
};

double Sync::SYNC = 0;

class Timeout: public Event{
public:
    Timeout(): Event(timeout, 0) {}

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
    int destination;
    bool isAck; // true acknowledgement, false datapacket

public:
    Packet(int dest, bool ack): destination(dest), isAck(ack){}
};

class Host { 
    static int NumHosts; // need to know this in order to create random destination
    static int T;        // maximum backoff given no retransmissions
    // static array so we can implement collision avoidance
    static int* backoff; // doing it in syc tics versus real time because easier for conflict avoidance
    // backoff < 0 means nothing in queue (nothing to transmit)
    // backoff > 0 means waiting to transmit
    // backoff == 0 means either transmitting or waiting for ack

    int hostID;  // position in hosts array, maybe don't need this, but might if put create packets and stuff
    int tmNum; // transmission number, max tmNum = 1 + maxRTM.  Max backoff = tmNum * T
    std::queue<Packet*> packetQueue; // i think its initialized implicitly

public:
    // initially set backoff to (-1) to show that nothing in queue
    Host(int id): hostID(id), tmNum(0){
        backoff[id] = -1;
    }

    // initialize static variables
    static void initHosts(int N, int t)
    {
        NumHosts = N;
        backoff = new int[N];
        T = t;
    }

    void enqueueDataPacket()
    {
        packetQueue.push(new Packet(randomDestination(hostID, NumHosts), false));
        // if nothing ready to transmit, as denoted by a negative backoff value
        // then need to set a new backoff value for this packet
        if (backoff < 0)
            backoff[hostID] = generateRandomBackOff(T, backoff, NumHosts);
    }
    void enqueueAckPacket(int dest)
    {
        packetQueue.push(new Packet(dest, true));
        // if nothing ready to transmit, as denoted by a negative backoff value
        // then need to set a new backoff value for this packet
        if (backoff < 0)
            backoff[hostID] = generateRandomBackOff(T, backoff, NumHosts);

    }

    // decrements backoff value if it is larger than zero
    // returns true if this act makes the value 0, and thus the Host is ready to transmit
    bool decrementBackoff()
    {
        if (backoff[hostID] > 0)
        {
            --(backoff[hostID]);
            if(backoff[hostID] == 0)
                return true;
        }
        return false;
    }
    
};

int* Host::backoff = NULL;
int Host::NumHosts = 0;
int Host::T = 0;



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
    Event* e;                       // holds the event currently being manipulated

    // initialize static variables of events
    Arrival::setLambda(lambda);
    Sync::setSYNC(SYNC);
    Host::initHosts(N, T);


    // initialize each host and create its initial arrival event
    for (int i = 0; i < N; i++)
    {
        hosts[i] = new Host(i);
        eventList->insert(new Arrival(time, i));

    }

    eventList->insert(new Sync(time));  // create the first sync event


    for(int i = 0; i < eventsSimulated; i++)
    {
        // pop event to handle
        e = eventList->removeFirst();

        // update time
        time = e->getEventTime();

        if (e->getType() == arrival)
        {
            // cast to arrival pointer
            Arrival *a = static_cast<Arrival*>(e);

            // check if cast actually worked
            if (a)
            {
                // need to create a new arrival event for the previous arrival event's host
                eventList->insert(new Arrival(time, a->getHost()));

                // following line testing behaviour of arrival event
                //std::cout << "process arrival event for host: " << a->getHost() << " at time: " << a->getEventTime() << std::endl;

                // now need to put packet in queue of host.
                // will generate length of packet on demand when create a departure event
                // but need to indicate that it is not a ack packet
                hosts[a->getHost()]->enqueueDataPacket();

            }
            else // not actually an arrival pointer
            {   
                std::cerr << "error: process event of arrival type that wasn't actually an arrival event" << std::endl;
            }
        }

        
        else if (e->getType() == departure)
        {
            // cast to departure pointer
            Departure *d = static_cast<Departure*>(e);

            // check if cast actually worked
            if (d)
            {


            }
            else // not actually a departure pointer
            {   
                std::cerr << "error: process event of departure type that wasn't actually a departure event" << std::endl;
            }

        }
        else if (e->getType() == sync)
        {
            // cast to Sync pointer
            Sync *s = static_cast<Sync*>(e);

            // check if cast actually worked
            if (s)
            {
                // need to create a new Sync event
                eventList->insert(new Sync(time));

                //std::cout << "process sync event at time: " << s->getEventTime() << std::endl;

                // if channel is free, go through all hosts and decrement backoff.
                // if backoff reaches zero, set channel to busy and create departure event
                // also continue to decrement the rest of the backoffs to help with collision avoidance
                if (channelBusy == false)
                {
                    for (int i = 0; i < N; i++)
                    {
                        hosts[i]->decrementBackoff();
                    }
                }


            }
            else // not actually a sync pointer
            {   
                std::cerr << "error: process event of sync type that wasn't actually a sync event" << std::endl;
            }

        }
        else if (e->getType() == timeout)
        {
            // cast to arrival pointer
            Timeout *t = static_cast<Timeout*>(e);

            // check if actually worked
            if (t)
            {

            }
            else // not actually a timeout pointer
            {   
                std::cerr << "error: process event of timeout type that wasn't actually a timeout event" << std::endl;
            }
        }

        // free memory of processed event
        delete e;


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



