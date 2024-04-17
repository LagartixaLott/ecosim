#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <barrier>

int n_execucao=0;
static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

bool alive(entity_t bicho){

}
// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

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

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    pos_t posicao;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;
bool iteracao(entity_t entidade){
    
    //tratar primeiro as mortes, alimentações e reproduções
    //por último se ele vai andar
    //deve haver um lock no inicio 
    //deve haver um wait() caso a ação seja de reprodução ou andar
    while(alive(entidade)){
    cd.wait(execucao_);
    execucao_.lock();
    if(entidade.type=plant){

    }
    if(entidade.type=herbivore){

    }
    if(entidade.type=carnivore){

    }

    execucao_.unlock();
    (*ponteiro_barreira).arrive_and_wait();
    }
    (*ponteiro_barreira).arrive();
    n_threads--;
}

int paralelo;
auto ponteiro_barreira = new std::barrier(1);
std::mutex mutex_morte;
std::mutex mutex_come;
std::mutex mutex_reproduz;
std::mutex mutex_anda;
std::mutex execucao;
std::unique_lock execucao_(execucao);
std::condition_variable cd;
int n_threads=0;
int main(){
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        // <YOUR CODE HERE>
        std::random_device rd;
        std::mt19937 posicao_aleatoria(rd()); 
        std::uniform_int_distribution<> distrib(0,15);
        for(int i=0;i<(uint32_t)request_body["plants"];i++){
            pos_t posicao(distrib(posicao_aleatoria),distrib(posicao_aleatoria));
            if(entity_grid[posicao.i][posicao.j].type==empty){
                entity_t planta;
                planta.age=0;
                planta.energy=100;
                planta.type=plant;
                planta.posicao= posicao;
                entity_grid[posicao.i][posicao.j]=planta;
                std::thread t(iteracao,planta);
                n_threads++;
                 t.detach();
                
            }
            else{
                i--;
            }
        }
         for(int i=0;i<(uint32_t)request_body["herbivores"];i++){
            pos_t posicao(distrib(posicao_aleatoria),distrib(posicao_aleatoria));
            if(entity_grid[posicao.i][posicao.j].type==empty){
                entity_t herbivoro;
                herbivoro.age=0;
                herbivoro.energy=100;
                herbivoro.type=herbivore;
                herbivoro.posicao=posicao;
                entity_grid[posicao.i][posicao.j]=herbivoro;
                std::thread t(iteracao,herbivoro);
                t.detach();
                n_threads++;
               
            }
            else{
                i--;
            }
        }
         for(int i=0;i<(uint32_t)request_body["carnivores"];i++){
            pos_t posicao(distrib(posicao_aleatoria),distrib(posicao_aleatoria));
            if(entity_grid[posicao.i][posicao.j].type==empty){
                entity_t carnivoro;
                carnivoro.age=0;
                carnivoro.energy=100;
                carnivoro.type=carnivore;
                carnivoro.posicao=posicao;
                entity_grid[posicao.i][posicao.j]=carnivoro;
                std::thread t(iteracao,carnivoro);
                n_threads++;
                 t.detach();
            }
            else{
                i--;
            }
        }
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        // <YOUR CODE HERE>
        int num_threads_aux= n_threads;
        std::barrier barreira_fim(num_threads_aux);
        ponteiro_barreira=&barreira_fim;
        cd.notify_all();
      
       (*ponteiro_barreira).arrive_and_wait();

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}