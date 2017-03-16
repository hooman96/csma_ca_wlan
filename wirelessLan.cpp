#include <iostream>
#include <list>
#include <string>
#include <queue>
#include <algorithm> // max
#include <cmath>
#include <math.h>
#include <random> // for hooman random stuff


//  these should be constant for our project
    const double maxRTM = 3;              // maximum number of retransmissions
    const int maxPktSize = 1544;          // maximum size of a packet in bytes
    const int ackPktSize = 64;            // acknowledgement packet size in bytes
    const double channelCapacity = 11000000.0;    // 11 Mbps (bits)
    const double SIFS = 0.00005;                  // 0.05 msec, delay before ack
    const double DIFS = 0.0001;                   // 0.1 msec, delay before send
    const double SYNC = 0.00001;                  // 0.01 msec


// randomly calculates negative-exponenetially-distributed-time
double nedt(double rate)
{
     double u;
     u = drand48();
     return ((-1/rate)*std::log(1-u));
}

double dataLengthFrame(double rate) 
{
    // http://en.cppreference.com/w/cpp/numeric/random/exponential_distribution
    std::random_device rd;
    std::mt19937 gen(rd());

    std::exponential_distribution<> d(1); // generate nedt between 0 and 1

    return int(maxPktSize * d(gen));
}

double transmissionTime(int bytes)
{
    return (bytes * 8) / (channelCapacity);
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
	Event(eventtype type, double etime): eventTime(etime), type(type) {}
    virtual ~Event(){}

	double getEventTime() {
		return eventTime;
	}

    eventtype getType()
    {
        return type;
    }

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

};

double Arrival::lambda = 0;

class Departure: public Event {

    // shape of packet size distribution
    static double mu;

    bool ack;         // denotes if it is an acknowedgement packet
    int source;         // source host
    int destination;    // destination host
    int packetID;       // id of packet, see host class
    int size;           // paket size in bytes, used to determine throughput

public:
    Departure(double stime, int s, int d, int id, bool a): Event(departure, stime), ack(a), source(s), destination(d), packetID(id) 
    {
        // if ack packet, set the time to be that size
        // time already current time, so just need to add the new time.
        if(ack)
        {
            // this shouldn't be how it works in real life
            // we're supposed to scan the channel during sifs, 
            // then if clear set channel to busy and transmit.  
            // For this project TA say just add SIFS, which is easier so OK then
            size = ackPktSize;
            eventTime += (SIFS + transmissionTime(size));
        }
        else
        {
            //  same comment as above, except DFIS
            size = dataLengthFrame(mu);
            eventTime += (DIFS + transmissionTime(size));

        }

    }

    static void setMu(double m)
    {
        mu = m;
    }

    bool isAck()
    {
        return ack;
    }

    int getSource()
    {
        return source;
    }
    int getDestination()
    {
        return destination;
    }
    int getPacketID()
    {
        return packetID;
    }
    int getSize()
    {
        return size;
    }


};

double Departure::mu = 0;

class Sync: public Event {

    static double SYNC;

public:
    Sync(double stime): Event(sync, stime + SYNC) {}

    static void setSYNC(double s)
    {
        SYNC = s;
    }

};

double Sync::SYNC = 0;

class Timeout: public Event{

    static double to_time;

    int host;
    int timeoutID;
public:
    Timeout(double stime, int h, int id): Event(timeout, stime + to_time), host(h), timeoutID(id) {}

    static void setTO(double t)
    {
        to_time = t;
    }


    int getHost()
    {
        return host;
    }

    int getTimeoutID()
    {
        return timeoutID;
    }

};

double Timeout::to_time = 0;

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
    int  ackID; // used to make sure ack makes sense and stuff
    double queueTime; // time when packet first queued, used for statistics (Network delay)


    friend class Host;

public:
    Packet(double t, int dest, bool ack, int id = 0): destination(dest), isAck(ack), ackID(id), queueTime(t){}


};

class Host { 
    static int NumHosts; // need to know this in order to create random destination
    static int T;        // maximum backoff given no retransmissions
    // static array so we can implement collision avoidance
    static int* backoff; // doing it in syc tics versus real time because easier for conflict avoidance
    // backoff < 0 means nothing in queue (nothing to transmit)
    // backoff > 0 means waiting to transmit
    // backoff == 0 means either transmitting or waiting for ack

    int packetID; // the number of the packet sent.  
    // Not worring about overflow, and even it it does, it should still work correctly.
    // Used to cordinate acks and timeouts.
    // If a timeout occurs, there's a chance that there will be acks in the network 
    // that don't refer to the most recent transmission.

    int droppedPackets; // not necessary for our project, but i think it might be interesting to keep track of
    int hostID;  // position in hosts array, maybe don't need this, but might if put create packets and stuff
    int tmNum; // transmission number, max tmNum = 1 + maxRTM.  Max backoff = tmNum * T

    double retransmitTime; // used to calculate delay when there has been a retransmission
    double delay;  // total delay, used for statistics.
    std::queue<Packet> packetQueue; // i think its initialized implicitly

public:
    // initially set backoff to (-1) to show that nothing in queue
    Host(int id): packetID(0), droppedPackets(0),  hostID(id), tmNum(0), retransmitTime(0), delay(0){
        backoff[id] = -1;
    }

    // initialize static variables
    static void initHosts(int N, int t)
    {
        NumHosts = N;
        backoff = new int[N];
        T = t;
    }

    double getDelay()
    {
        return delay;
    }

    int getDropedPackets()
    {
        return droppedPackets;
    }

    void enqueueDataPacket(double stime)
    {
        packetQueue.push(Packet(stime, randomDestination(hostID, NumHosts), false));
        // if nothing ready to transmit, as denoted by a negative backoff value
        // then need to set a new backoff value for this packet.
        // in real life I don't think the first packet waits for a backoff,
        // but the TAs told us to do it this way.
        if (backoff[hostID] < 0)
            backoff[hostID] = generateRandomBackOff(T, backoff, NumHosts);
    }
    void enqueueAckPacket(double stime, int dest, int ackID)
    {

        // In real life I don't think the ack goes in the back of the queue, 
        // But the TAs told us to do it this way.

        packetQueue.push(Packet(stime, dest, true, ackID));
        // if nothing ready to transmit, as denoted by a negative backoff value
        // then need to set a new backoff value for this packet
        if (backoff[hostID] < 0)
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


    void receiveAck(int AckID)
    {
        // if correct ack, can pop packet from start of queue
        if (AckID == packetID)
        {
            //std::cerr << "receive correct ack, Host: " << hostID << ", AckID: " << AckID << std::endl;
            packetQueue.pop(); // pop packet from queue
            packetID++; // new packet to send, so increment packetID
            tmNum = 0; // need to reset TmNum because new packet to transmit.
            // if no more packets in queue, indicate it by setting backoff id to -1
            if (packetQueue.empty())
            {
                backoff[hostID] = -1;
            }
            // eles if still packet to send, set new backoff value
            else
            {
                backoff[hostID] = generateRandomBackOff(T, backoff, NumHosts);
            }


        } 
        // if AckID does not match PacketID do nothing
        // should never get out of order ack because can only sent 1 packet at a time
    }

    void receiveTimeout(double stime, int TO_ID)
    {
        // if timeout refers to current packet, need to resend with larger backoff
        if (TO_ID == packetID)
        {
            // if haven't reached maximum transmissions yet, need to retransmit it by resetting backoff value
            // tmNum refers to current transmission.  On transmission 3, there have been 2 retransmissions
            // if MaxRTM = 3, then should be able to send another one.
            // if MaxRTM = 3 and tmNum = 4, then there have already been 3 retransmissions and need to abort
            if (tmNum <= maxRTM)
            {
                // need to reset packet queue time because should not double count delay when waiting for ack
                // actually, i don't want to make it a queue of pointers so i'll do this hack instead
                retransmitTime = stime;

                // need to increase mack backoff by a multiple of (tmNum + 1),
                // since tmNum in incremented when departure event created, but need to use that as a multiplyer here
                backoff[hostID] = generateRandomBackOff(T * (tmNum + 1), backoff, NumHosts);
            }
            // else need to drop packet.  Do this by pretending to ack it
            else
            {
                droppedPackets++;
                receiveAck(packetID);

            }

        }

    }


    // performs packet processing and prepares packet for departure
    // returns a departure event 
    // not going to do error checking, so assumes that there is at least 1 packet in the queue and that hopeufully backoff == 0
    // actually, maybe will do error checking
    Departure* createDeparture(double stime)
    {
        // i lied, error checking
        if (backoff[hostID] != 0)
            std::cerr << "Host creating Departure event when backoff != 0" << std::endl;
        // the other one should cause runtime issues if bug, so won't check for it
        // no that's stupid
        if (packetQueue.empty())
        {
            std::cerr << "Host creating Departure event when queue empty" << std::endl;

            return NULL;
        }



        // get packet info
        Packet p = packetQueue.front();

        Departure* depart; // holds return value


        //std::cerr << "creating departure with destination: " << p.destination << std::endl;

        // if an ack packet, create ack event
        // since we don't need to wait for ack, can immediatley pretend we got one
        if (p.isAck)
        {
            receiveAck(packetID);
            depart =  new Departure(stime, hostID, p.destination, p.ackID, true);
        }
        // else need to create packet and increment tmNum
        else
        {
            depart = new Departure(stime, hostID, p.destination, packetID, false);
            tmNum ++;


        }

        // calculate delay
        // if this is a retransmission (tmNum > 1), then need to use retransmitTime as a base
        if (tmNum > 1)
        {
            delay += (depart->getEventTime() - retransmitTime);
        }
        // else use packet time as a base
        else
        {
            delay += (depart->getEventTime() - p.queueTime);
        }

        return depart;

    }

    Timeout* createTimeout(double stime)
    {
        return new Timeout(stime, hostID, packetID);
    }
    
};

int* Host::backoff = NULL;
int Host::NumHosts = 0;
int Host::T = 0;



int	main(int argc, char const *argv[])
{

//  read from command line, but given default values
    double lambda = 0.01;    // dexcribes shape of arrival distribution
    double mu = 1;          // describes shape of distribution of PktSize (r)
    int N = 20;             // number of hosts in network.
    int T = 400;            // maximum backoff value in number sync cycles. Should be larger than N I suppose.
    double TO = 0.005; // for project, will take values of 5, 10, or 15 msec. 
    int eventsSimulated = 1000000; // the bound of for loop

//  these are variables used throught the program
    double time = 0;        // time simulated since start of simulation in seconds
    double transmitted = 0; // number of bytes successfully transmitted (will include ack bytes)
    double delay = 0;       // queue + transmission delay in seconds
    int packets = 0;
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
                     "-t: timeout(" << TO << "), given in msec\n" 
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
                TO = std::stod(argv[i+1]);

            }
            catch(std::exception e)
            {
                std::cerr << "invalid input for -t, using default value " << TO << std::endl;
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
                     "-t: timeout(" << TO << "), given in msec\n" << std::endl;
                     "-s: eventsSimulated(" << eventsSimulated << "), controls length of simulation\n" << std::endl;*/




    // Now the simulation can finally begin

    hosts = new Host*[N];           // create an array to hold all Hosts
    eventList = new GEL();          // create a list of events
    Event* e;                       // holds the event currently being manipulated

    // initialize static variables of events
    Arrival::setLambda(lambda);
    Departure::setMu(mu);
    Sync::setSYNC(SYNC);
    Host::initHosts(N, T);
    Timeout::setTO(TO);


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
                hosts[a->getHost()]->enqueueDataPacket(time);

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
                // keep track of bytes transmitted
                transmitted += d->getSize();
                packets ++;


                // if an ack departure, need to notify receiving host
                if (d->isAck())
                {
                    // using an integer for packet IDs
                    hosts[d->getDestination()]->receiveAck(d->getPacketID());

                }
                // if a data departure, need to create ack packet in destination queue
                else
                {
                    hosts[d->getDestination()]->enqueueAckPacket(time, d->getSource(), d->getPacketID());
                }
                // set channel to free
                channelBusy = false;


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
                    // possible host that needs to transmit
                    int hostToProcess = -1;
                    for (int i = 0; i < N; i++)
                    {
                        // decrements backoff value, returns true if backoff becomes zero
                        // need to save host index if it needs to be processed
                        // since we provide collision detection, there should only ever be one host ready to process
                        if(hosts[i]->decrementBackoff())
                        {
                            hostToProcess = i;
                        }

                        // if a host was selected to process, need to process it

                    }
                    if (hostToProcess >= 0)
                    {
                        // have the host create a departure event
                        Departure* departHelp = (hosts[hostToProcess]->createDeparture(time));
                        if (departHelp)
                        {
                            eventList->insert(departHelp);
                            // create a timeout event tied to the host, but only if not ack
                            if (!departHelp->isAck())
                            {
                                eventList->insert(hosts[hostToProcess]->createTimeout(time));

                            }
                            // set channel to busy
                            channelBusy = true;
                        }
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
                // tell host that timeout event occured
                hosts[t->getHost()]->receiveTimeout(time, t->getTimeoutID());

            }
            else // not actually a timeout pointer
            {   
                std::cerr << "error: process event of timeout type that wasn't actually a timeout event" << std::endl;
            }
        }

        // free memory of processed event
        delete e;


    }

    int drop = 0;

    for (int i = 0; i < N; i++)
    {
        delay += hosts[i]->getDelay();
        drop += hosts[i]->getDropedPackets();
    }

    std::cout << "lambda: " << lambda << std::endl;
    std::cout << "T: " << T << std::endl;
    std::cout << "N: " << N << std::endl;

    std::cout << "Throughput: " << transmitted / time << " Bps" << std::endl;
    std::cout << "Average Network Delay: " << delay / transmitted << " s/B" << std::endl; // I changed this to make sense
    // std::cout << "Average Network Delay: " << delay / packets << " s/packet" << std::endl; // I changed this to make sense
    // std::cout << "Average Network Delay (per instructions): " << delay / (transmitted/time) << " s^2/B" << std::endl; // This is what doesn't make sense

    std::cout << "Packets Dropped: " << drop << std::endl;
    std::cout << "Packets Sent: " << packets << std::endl;

    std::cout << "------------------------------------------------------" << std::endl;

    // delete dynamically allocated data
    for (int i = 0; i < N; i++)
    {
        delete hosts[i];
    }
    delete hosts;
    delete eventList;

}