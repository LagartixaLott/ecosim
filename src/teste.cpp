#include <random>
using namespace std;

struct pos_t
{
    uint32_t i;
    uint32_t j;
    pos_t(int i_,int j_){
        this->i=i_;
        this->j=j_;
    }
    pos_t(){
        this->i=0;
        this->j=0;
    }
};

int main(){
    std::random_device rd;
    std::mt19937 rand_pos(rd()); 
    std::uniform_int_distribution<> distrib(0,15);
    pos_t jo(distrib(rand_pos) , distrib(rand_pos));
    fprintf(stdout , "Coord X : %d /// Coord Y : %d" , jo.i , jo.j);
}