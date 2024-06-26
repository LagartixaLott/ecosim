//MAIS ANTIGO, consertado

#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stack>
//#include <barrier>
int n_threads = 0;
int n_ready_threads = 0;
std::mutex m;
//Número de plantas em execução
int num_threads_p = 0;
//Número de herbívoros em execução
int num_threads_h = 0;
//Número de carnívoros em execução
int num_threads_c = 0;

//autorizar todas as entidades a checar se irão morrer de idade
std::condition_variable iteration_ages;

//vez da iteração da planta
std::condition_variable iteration_p;
//vez da iteração do herbívoro
std::condition_variable iteration_h;
//vez da iteração do carnívoro
std::condition_variable iteration_c;

//toda vez que alguma ação é finalizada, avisa ao main
std::condition_variable thread_ready;


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
    pos_t(uint32_t i_, uint32_t j_): i(i_), j(j_) {};
};




struct entity_t{

    entity_type_t type;
    int32_t energy;
    int32_t age;

    //true caso tenha sido morto por alguma entidade na iteração
    bool killed = false;

    // ao morrer, esvazia os campos da sua posição no array entity_grid, e decrementa a quantidade do seu tipo
    void die();

    // retorna a idade máxima do seu tipo
    int32_t max_age();

    // retorna o tipo de presa do seu tipo
    entity_type_t prey_type(); 
    
    // incrementa a variável que indica a quantidade de threads/entidades do seu tipo (ao nascer)
    void inc_num_thread_t();
    // decrementa a variável que indica a quantidade de threads/entidades do seu tipo (ao morrer)
    void dec_num_thread_t();

    //Probabilidade:

    // probabilidade de reproduzir
    double prob_rep();
    // probabilidade de comer
    double prob_eat();
    // probabilidade de mover
    double prob_mov();

    //Energia:

    // mínimo de energia para reproduzir
    int32_t min_rep() { return (this->type == plant) ? 0 : THRESHOLD_ENERGY_FOR_REPRODUCTION;}; // retorna 0 se planta, THRESHOLD_ENERGY_FOR_REPRODUCTION caso contrário

    // custo de energia para reproduzir
    int32_t cost_rep() {return (this->type == plant) ? 0 : 10;}; // retorna 0 se planta, 10 caso contrário

    // ganho de energia ao comer
    int32_t gain_eat() {return (this->type == herbivore) ? 30 : 20;}; // retorna 30 se herbivoro, 20 caso carnivoro. Plantas não comem.

    // custo de energia para mover-se
    int32_t cost_move(){ return 5;}; // poderia colocar como const... mas deixei assim para seguir o padrão

    // faz uma analise das posições adjacentes. Retorna um vetor com as posições 
    std::vector<pos_t> close_pos(pos_t pos, entity_type_t find_type); 
  
};

static std::vector<std::vector<entity_t>> entity_grid;

//FUNÇÕES EXPORTAÇÃO
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
//FUNÇÕES EXPORTAÇÃO FIM



//IMPLEMENTAÇÃO DO ENTITY_T INI

// retorna a idade máxima do seu tipo
int32_t entity_t::max_age(){
    switch (this->type){
        case plant: return PLANT_MAXIMUM_AGE;
        case herbivore: return HERBIVORE_MAXIMUM_AGE;
        case carnivore: return CARNIVORE_MAXIMUM_AGE;
    }
    return 0;
}
// retorna o tipo de presa do seu tipo
entity_type_t entity_t::prey_type(){
    switch (this->type){
        case herbivore: return plant;
        case carnivore: return herbivore;
    }
    return empty;
}

// incrementa o número de threads correspondente ao seu tipo (ao nascer)
void entity_t::inc_num_thread_t(){
    switch(this->type){
        case plant: num_threads_p++; break;
        case herbivore: num_threads_h++; break;
        case carnivore: num_threads_c++; break;
    }
}
// decrementa o número de threads correspondente ao seu tipo (ao morrer)
void entity_t::dec_num_thread_t(){
    switch(this->type){
        case plant: num_threads_p--; break;
        case herbivore: num_threads_h--; break;
        case carnivore: num_threads_c--; break;
    }
}
// ao morrer, esvazia os campos da sua posição no array entity_grid, e decrementa a quantidade do seu tipo
void entity_t::die(){
    this -> dec_num_thread_t();
    this -> type = empty;
    this -> age = 0;
    this -> energy = 0;
    this -> killed = false;
}

// probabilidade de reproduzir
double entity_t::prob_rep(){
    switch (this->type){
        case plant: return PLANT_REPRODUCTION_PROBABILITY;
        case herbivore: return HERBIVORE_REPRODUCTION_PROBABILITY;
        case carnivore: return CARNIVORE_REPRODUCTION_PROBABILITY;
    }
    return 0;
}
// probabilidade de comer
double entity_t::prob_eat(){
    switch (this->type){
        case herbivore: return HERBIVORE_EAT_PROBABILITY;
        case carnivore: return CARNIVORE_EAT_PROBABILITY;
    }
    return 0;
}
// probabilidade de mover
double entity_t::prob_mov(){
    switch (this->type){
        case herbivore: return HERBIVORE_MOVE_PROBABILITY;
        case carnivore: return CARNIVORE_MOVE_PROBABILITY;
    }
    return 0;
}

std::vector<pos_t> entity_t::close_pos(pos_t pos, entity_type_t find_type){
    std::vector<pos_t> close_pos;
    close_pos.clear();
    int i = pos.i;
    int j = pos.j;
    if(i<NUM_ROWS-1){
        if(entity_grid[i+1][j].type == find_type && entity_grid[i+1][j].killed == false){close_pos.push_back(pos_t(i+1,j));}
    }
    if(j<NUM_ROWS-1){
        if(entity_grid[i][j+1].type == find_type && entity_grid[i][j+1].killed == false){close_pos.push_back(pos_t(i,j+1));}
    }
    if(i>0){
        if(entity_grid[i-1][j].type == find_type && entity_grid[i-1][j].killed == false){close_pos.push_back(pos_t(i-1,j));}
    }
    if(j>0){
        if(entity_grid[i][j-1].type == find_type && entity_grid[i][j-1].killed == false){close_pos.push_back(pos_t(i,j-1));}
    }
    return close_pos;
}
//IMPLEMENTAÇÃO ENTITY_T FIM

//AUXILIARES:

// retorna um inteiro de 0 a int max_
uint32_t random_integer(int max_) {
    return rand() % max_ + 1;;
}
//realiza o "sorteio" de acordo com a probabilidade
bool random_action(double probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}
//AUXILIARES FIM


void iteracao(pos_t pos, entity_type_t type){
    pos_t pos_cur=pos;
    bool isAlive = true;
    bool isDying = false;
    entity_t* entity = &entity_grid[pos.i][pos.j];
    while(isAlive){

        // Cria um objeto do tipo unique_lock que no construtor chama m.lock()
		std::unique_lock<std::mutex> ni_lk(m);

        iteration_ages.wait(ni_lk);

        // atualiza os valores de entidade
        entity = &entity_grid[pos_cur.i][pos_cur.j];
        // atualiza a idade
        entity -> age = entity-> age + 1;

        //confere a idade
        if (entity->age > entity->max_age()){
            entity -> die();
            isDying = true;
        }

        n_ready_threads++;
        thread_ready.notify_one(); // notifica ao main que já terminou de conferir a morte por idade.

        if(isDying) {
            isAlive = false;
            break; // sai da função
        }


        // caso esteja vivo, avisa que está pronto e espera o notify do seu tipo.

        switch(type){
        case plant:
            // caso planta, espera a chamada de plantas (após a de herbívoros)
            iteration_p.wait(ni_lk);
            break;
        case herbivore:
            // caso herbívoro, espera a chamada de herbívoro (após a de carnívoros)
            iteration_h.wait(ni_lk);
            break;
        case carnivore:
            // caso carnívoro, espera a chamada de carnívoro (a primeira após a checagem de idade das entidades)
            iteration_c.wait(ni_lk); 
            break;
        }

        // atualiza os valores de entidade
        entity = &entity_grid[pos_cur.i][pos_cur.j];

        // caso ele tenha sido morto por alguma entidade/thread anterior
        if(entity->killed == true){ 
            entity->die();
            isDying = true;
        }

        // se vivo
        if(!isDying) {
        
            
            // recebe as posições adjacentes em que há alguma presa. No caso de carnívoro: herbívoro; caso herbívoro, planta.
            std::vector<pos_t> preys = entity->close_pos(pos_cur, entity->prey_type());
            
            // comer
            while(preys.size()>0){ //confere cada um das prezas que estiverem adjacentes
                pos_t it_pos = preys.back(); // pega o ultimo da fila.
                entity_t* ent_adj = &entity_grid[it_pos.i][it_pos.j]; // ponteiro auxiliar, da entidade (presa) da posição correspondente
                
                if(random_action(entity->prob_eat())) {
                    //matar
                    ent_adj->killed = true;
                    //ganha energia ao matar
                    entity->energy += entity->gain_eat();
                    if(entity->energy >= MAXIMUM_ENERGY) entity->energy = MAXIMUM_ENERGY;
                }
                preys.pop_back();//matando ou não, retira da fila
            }

            // recebe as posições adjacentes vazias
            std::vector<pos_t> pos_aval = entity->close_pos(pos_cur, empty);

            // reproduzir
            if(entity->energy>20){
            if(pos_aval.size() > 0 && random_action(entity->prob_rep())){
                int item = random_integer(pos_aval.size())-1;
                pos_t it_pos = pos_aval.at(item); //posição aleatoria
                pos_aval.erase(pos_aval.begin()+item); // ao reproduzir, remove o local entre os locais disponíveis.
                entity_t* baby_entity= &entity_grid[it_pos.i][it_pos.j];
                baby_entity->type=type;
                baby_entity->energy=100;
                baby_entity->age=0;
                std::thread t(iteracao, it_pos,type);
                t.detach();
                baby_entity->inc_num_thread_t(); //incrementa o número de threads correspondente ao seu tipo. 
                entity->energy -= entity->cost_rep();
            }
            }
            // mover
            if(entity->energy >= entity->cost_move()){
            if(pos_aval.size() > 0 && random_action(entity->prob_mov())){
                pos_t new_pos(500,500);
                entity_t* new_pos_entity;
                // variável auxiliar new_pos. Escolhe aleatoriamente uma das posições vazias presentes no vetor
                new_pos = pos_aval.at(random_integer(pos_aval.size())-1); //carrega a posição aleatoria

                // variável auxiliar new_pos_entity. É o endereço da posição vizinha escolhida. Excluída ao final do if.
                new_pos_entity = &entity_grid[new_pos.i][new_pos.j];
                
                // preenche as informações da posição nova segundo a atual/antiga.
                new_pos_entity -> age = entity -> age;
                new_pos_entity -> energy = entity -> energy;
                new_pos_entity -> type = entity -> type;
                
                // esvazia a posição antiga
                entity -> type = empty;
                entity -> age = 0;
                entity -> energy = 0;

                // atualiza a variável do thread entity, atribuindo/recendo o endereço da posição em que moveu.
                entity = new_pos_entity;
                
                pos_cur.i = new_pos.i;
                pos_cur.j = new_pos.j;
                
                entity->energy -= entity-> cost_move();
            }

            // confere a energia
            if (entity->energy <= 0){
                entity->die();
                isDying = true;
            }

        }
        }
        n_ready_threads++;
        thread_ready.notify_one(); // avisa ao main que terminou a ação / iteração

        if(isDying) {
            isAlive = false;
            break; // sai da função
        }
    }
    
}

bool running = false;

int main()
{
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

        std::unique_lock<std::mutex> lk(m);
        running = false;
        thread_ready.notify_one();

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

        num_threads_c = 0; // reseta a quantidade de threads rodando
        num_threads_h = 0; // reseta a quantidade de threads rodando
        num_threads_p = 0; // reseta a quantidade de threads rodando
        
        int num_p = (uint32_t)request_body["plants"];
        int num_h = (uint32_t)request_body["herbivores"];
        int num_c = (uint32_t)request_body["carnivores"];

        for (int i = 0; i<num_p+num_h+num_c; i++){
            pos_t pos(random_integer(NUM_ROWS-1), random_integer(NUM_ROWS-1));
            entity_t* baby_entity;
            baby_entity = &entity_grid[pos.i][pos.j];
            if(baby_entity -> type == empty){
                if(i<num_p){ baby_entity -> type = plant;}
                else if(i<num_p+num_h){ baby_entity -> type = herbivore;}
                else{ baby_entity -> type = carnivore;}
                baby_entity -> energy = 100; //não tem problema a planta ter energia, não gasta
                baby_entity -> age = 0;
                std::thread t(iteracao, pos,baby_entity->type);
                 t.detach();
                baby_entity -> inc_num_thread_t();
            }
            else{
                i--;
            }
        }

        running = true;


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
        
        std::unique_lock<std::mutex> tf_lk(m);
        // quantiadade de threads (c+h+p) ativos no momento do new_iteration.notify_all();
        int num_threads_aux  = num_threads_c + num_threads_h + num_threads_p; 
        

        // avisa a todos que o get foi solicitado
        iteration_ages.notify_all();
        n_ready_threads = 0;

        // aguarda até que todas as entidades tenham checado suas idades
        while(n_ready_threads < num_threads_aux && running) {
            thread_ready.wait(tf_lk);   
        }

        // quantiadade de threads_c ativos ao término da checagem de idades
        int num_c_threads_aux  = num_threads_c; 
        // quantiadade de threads_h ativos ao término da checagem de idades
        int num_h_threads_aux  = num_threads_h;
        // quantiadade de threads_p ativos ao término da checagem de idades
        int num_p_threads_aux  = num_threads_p;

        //avisa a todos os carnívoros que chegou a sua vez
        iteration_c.notify_all();
        n_ready_threads = 0;

        //aguarda que todos os carnívoros realizem suas ações da iteração
        while(n_ready_threads < num_c_threads_aux && running) {
            thread_ready.wait(tf_lk);   
        }

        // avisa a todos os herbívoros que chegou a sua vez
        iteration_h.notify_all();
        n_ready_threads = 0;

        // aguarda que todos os herbívoros realizem suas ações da iteração
        while(n_ready_threads < num_h_threads_aux && running) {
            thread_ready.wait(tf_lk);
        }

        // avisa a todas as plantas que chegou a sua vez
        iteration_p.notify_all();
        n_ready_threads = 0;

        // aguarda que todas as plantas realizem suas ações da iteração
        while(n_ready_threads < num_p_threads_aux && running) {
            thread_ready.wait(tf_lk);
        }

        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}