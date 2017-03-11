#include <string>
#include <iostream>


enum type 
{
    arrival, departure, sync, timeout
};

class foo
{
public:
        int lol;

    foo()
    {
        lol =  10;
    }
};


int main (int argc, const char** argv)
{

    type t;
    t = departure;
    std::cout << "type: " << t << std::endl; 
    int N = 10;
        for (int i = 1; i + 1 < argc; i += 2)
    {
        if (std::string("-N") == argv[i])
            N = std::stoi(argv[i+1]);
    }
            std::cout << "N = " << N << std::endl;

            foo* list = new foo[N];
            N = 10;
            int list2[N];
            for (int i = 0; i < N; i ++)
            {
                std::cout << list[i].lol << std::endl;
            }

}