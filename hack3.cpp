
#include <random> // random host

#include <iostream>

using namespace std;

int randomDestination(int source, int N)
{
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


int main(int argc, const char* argv[])
{
    int N, h, T;

    cout << "enter nuber of hosts: " ;
    cin >> N ;
    cout << "enter maximum backoff: " ;
    cin >> T ;

    int data[N];
    for (int i = 0; i < N; i++)
    {
        data[i] = -1;
    }


    while (true)
    {
        cout << "enter source host: " ;
        cin >> h;
        data[h] = 0;
        data[h] = generateRandomBackOff(T, data, N);

        for (int i = 0; i < N; i++){
    
            cout << i << ": " << data[i] << endl;
        }

    }






    
return 0;
}
