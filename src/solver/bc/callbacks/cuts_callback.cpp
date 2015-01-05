#include <solver/bc/callbacks/cuts_callback.h>
#include <solver/bc/callbacks/feasibility_cuts_separator.h>
#include <solver/bc/callbacks/vi_separator_capacity.h>
#include <solver/bc/callbacks/vi_separator_fork.h>
#include <solver/bc/callbacks/vi_separator_generalised_order.h>
#include <solver/bc/callbacks/vi_separator_simplified_fork.h>
#include <solver/bc/callbacks/vi_separator_subtour_elimination.h>

#include <chrono>
#include <ctime>
#include <ratio>

IloCplex::CallbackI* cuts_callback::duplicateCallback() const {
    return (new(getEnv()) cuts_callback(*this));
}

void cuts_callback::main() {
    using namespace std::chrono;
    
    auto node_number = getNnodes();
    auto sol = compute_x_values();
    
    if(sol.is_integer) {
        auto start_time = high_resolution_clock::now();
        auto feas_cuts = feasibility_cuts_separator::separate_feasibility_cuts(g, gr, sol, x, eps);
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_fork_vi += time_span.count();

        if(DEBUG && feas_cuts.size() > 0) {
            std::cerr << "Adding " << feas_cuts.size() << " feasibility cuts" << std::endl;
        }

        for(auto& cut : feas_cuts) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_feasibility_cuts_added++;
        }
    }
    
    if(params.bc.subtour_elim.enabled && node_number % params.bc.subtour_elim.cut_every_n_nodes == 0) {
        auto sub_solv = vi_separator_subtour_elimination(g, params, sol, env, x, eps);
        auto start_time = high_resolution_clock::now();
        auto valid_cuts_1 = sub_solv.separate_valid_cuts();
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_subtour_elimination_vi += time_span.count();

        if(DEBUG && valid_cuts_1.size() > 0) {
            std::cerr << "Adding " << valid_cuts_1.size() << " subtour elimination cuts" << std::endl;
        }

        for(auto& cut : valid_cuts_1) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_subtour_elimination_vi_added++;
        }
    }
    
    if(params.bc.generalised_order.enabled && node_number % params.bc.generalised_order.cut_every_n_nodes == 0) {
        auto go_solv = vi_separator_generalised_order(g, sol, env, x, eps);
        auto start_time = high_resolution_clock::now();
        auto valid_cuts_2 = go_solv.separate_valid_cuts();
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_generalised_order_vi += time_span.count();

        if(DEBUG && valid_cuts_2.size() > 0) {
            std::cerr << "Adding " << valid_cuts_2.size() << " generalised order cuts" << std::endl;
        }

        for(auto& cut : valid_cuts_2) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_generalised_order_vi_added++;
        }
    }
    
    if(params.bc.capacity.enabled && node_number % params.bc.capacity.cut_every_n_nodes == 0) {
        auto cap_solv = vi_separator_capacity(g, sol, env, x, eps);
        auto start_time = high_resolution_clock::now();
        auto valid_cuts_3 = cap_solv.separate_valid_cuts();
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_capacity_vi += time_span.count();
    
        if(DEBUG && valid_cuts_3.size() > 0) {
            std::cerr << "Adding " << valid_cuts_3.size() << " capacity cuts" << std::endl;
        }
    
        for(auto& cut : valid_cuts_3) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_capacity_vi_added++;
        }
    }
    
    if(params.bc.simplified_fork.enabled && node_number % params.bc.simplified_fork.cut_every_n_nodes == 0) {
        auto sfork_solv = vi_separator_simplified_fork(g, sol, env, x, eps);
        auto start_time = high_resolution_clock::now();
        auto valid_cuts_4 = sfork_solv.separate_valid_cuts();
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_simplified_fork_vi += time_span.count();
        
        if(DEBUG && valid_cuts_4.size() > 0) {
            std::cerr << "Adding " << valid_cuts_4.size() << " simplified fork cuts" << std::endl;
        }
        
        for(auto& cut : valid_cuts_4) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_simplified_fork_vi_added++;
        }
    }
    
    if(params.bc.fork.enabled && node_number % params.bc.fork.cut_every_n_nodes == 0) {
        auto fork_solv = vi_separator_fork(g, sol, env, x, eps);
        auto start_time = high_resolution_clock::now();
        auto valid_cuts_5 = fork_solv.separate_valid_cuts();
        auto end_time = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(end_time - start_time);
        data.time_spent_separating_fork_vi += time_span.count();
        
        if(DEBUG && valid_cuts_5.size() > 0) {
            std::cerr << "Adding " << valid_cuts_5.size() << " fork cuts" << std::endl;
        }
        
        for(auto& cut : valid_cuts_5) {
            add(cut, IloCplex::UseCutForce).end();
            data.total_number_of_fork_vi_added++;
        }
    }
}

ch::solution cuts_callback::compute_x_values() const {
    auto n = g.g[graph_bundle].n;
    auto xvals = std::vector<std::vector<double>>(2*n+2, std::vector<double>(2*n+2, 0));
    auto is_integer = true;

    auto col_index = 0;
    for(auto i = 0; i <= 2 * n + 1; i++) {
        for(auto j = 0; j <= 2 * n + 1; j++) {
            if(g.cost[i][j] >= 0) {
                auto n = getValue(x[col_index++]);
                if(n > eps) {
                    if(n < 1 - eps) {
                        is_integer = false;
                    }
                    xvals[i][j] = n;
                } else {
                    xvals[i][j] = 0;
                }
            }
         }
    }
    
    return ch::solution(is_integer, xvals);
}